#include "confirmation_view.h"

#include <gui/elements.h>
#include <furi.h>


struct ConfirmationView {
    View* view;
    ConfirmationViewCallback ok_callback;
    ConfirmationViewCallback cancel_callback;
    void* callback_context;
};

typedef struct {
    const char* header;
    const char* text;
} ConfirmationViewModel;



static void confirmation_view_draw_callback(Canvas* canvas, void* _model) {
    ConfirmationViewModel* model = (ConfirmationViewModel*)_model;

    // Draw boundary
    canvas_draw_rframe(canvas, 0, 16, 128, 48, 3);
    canvas_draw_rframe(canvas, 1, 17, 126, 46, 2);

    // Header
    if(model->header) {
        canvas_set_font(canvas, FontPrimary);
        elements_multiline_text_aligned(canvas, 64, 24, AlignCenter, AlignTop, model->header);
    }

    // Text
    if(model->text) {
        canvas_set_font(canvas, FontSecondary);
        elements_multiline_text_aligned(canvas, 64, 42, AlignCenter, AlignTop, model->text);
    }

    // Draw buttons
    canvas_set_font(canvas, FontSecondary);
    elements_button_center(canvas, "OK");
    elements_button_left(canvas, "Back");
}

static bool confirmation_view_input_callback(InputEvent* event, void* context) {
    ConfirmationView* instance = context;

    bool consumed = false;

    if(event->type == InputTypeShort) {
        if(event->key == InputKeyOk) {
            if(instance->ok_callback) {
                instance->ok_callback(instance->callback_context);
            }
            consumed = true;
        } else if(event->key == InputKeyBack) {
            if(instance->cancel_callback) {
                instance->cancel_callback(instance->callback_context);
            }
            consumed = true;
        }
    }

    return consumed;
}

ConfirmationView* confirmation_view_alloc(void) {
    ConfirmationView* instance = malloc(sizeof(ConfirmationView));
    instance->view = view_alloc();
    view_allocate_model(instance->view, ViewModelTypeLocking, sizeof(ConfirmationViewModel));
    view_set_context(instance->view, instance);
    view_set_draw_callback(instance->view, confirmation_view_draw_callback);
    view_set_input_callback(instance->view, confirmation_view_input_callback);

    with_view_model(
        instance->view,
        ConfirmationViewModel * model,
        {
            model->header = NULL;
            model->text = NULL;
        },
        true);

    return instance;
}

void confirmation_view_free(ConfirmationView* instance) {
    view_free(instance->view);
    free(instance);
}

View* confirmation_view_get_view(ConfirmationView* instance) {
    return instance->view;
}

void confirmation_view_set_header(ConfirmationView* instance, const char* text) {
    with_view_model(
        instance->view,
        ConfirmationViewModel * model,
        { model->header = text; },
        true);
}

void confirmation_view_set_text(ConfirmationView* instance, const char* text) {
    with_view_model(
        instance->view,
        ConfirmationViewModel * model,
        { model->text = text; },
        true);
}

void confirmation_view_set_ok_callback(
    ConfirmationView* instance,
    ConfirmationViewCallback callback,
    void* context) {
    instance->ok_callback = callback;
    instance->callback_context = context;
}

void confirmation_view_set_cancel_callback(
    ConfirmationView* instance,
    ConfirmationViewCallback callback,
    void* context) {
    instance->cancel_callback = callback;
    instance->callback_context = context;
}