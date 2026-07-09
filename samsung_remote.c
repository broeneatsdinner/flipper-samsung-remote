#include <furi.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <input/input.h>
#include <infrared.h>
#include <infrared_worker.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    SamsungRemoteScreenHome,
    SamsungRemoteScreenPhysical,
} SamsungRemoteScreen;

typedef enum {
    SamsungRemoteHomePower,
    SamsungRemoteHomeSimulate,
} SamsungRemoteHomeItem;

typedef struct {
    FuriMessageQueue* event_queue;
    ViewPort* view_port;
    Gui* gui;
    InfraredWorker* ir_worker;
    SamsungRemoteScreen screen;
    SamsungRemoteHomeItem selected;
    bool running;
} SamsungRemoteApp;

typedef struct {
    const char* name;
    InfraredMessage message;
} SamsungIrCommand;

/*
 * Hardcoded v0 command table copied from remotes/samsung-tv-source.ir.
 *
 * Keep this table boring and explicit. A later version can replace it with
 * loading parsed commands from a .ir file without changing the app controls.
 */
static const SamsungIrCommand samsung_ir_commands[] = {
    {"POWER", {InfraredProtocolSamsung32, 0x00000007, 0x00000002, false}},
    {"Up", {InfraredProtocolSamsung32, 0x00000007, 0x00000060, false}},
    {"Down", {InfraredProtocolSamsung32, 0x00000007, 0x00000061, false}},
    {"Left", {InfraredProtocolSamsung32, 0x00000007, 0x00000065, false}},
    {"Right", {InfraredProtocolSamsung32, 0x00000007, 0x00000062, false}},
    {"Select", {InfraredProtocolSamsung32, 0x00000007, 0x00000068, false}},
    {"Return", {InfraredProtocolSamsung32, 0x00000007, 0x00000058, false}},
};

static const InfraredMessage* samsung_remote_find_message(const char* name) {
    for(size_t i = 0; i < COUNT_OF(samsung_ir_commands); i++) {
        if(strcmp(samsung_ir_commands[i].name, name) == 0) {
            return &samsung_ir_commands[i].message;
        }
    }

    return NULL;
}

static void samsung_remote_send(SamsungRemoteApp* app, const char* name) {
    const InfraredMessage* message = samsung_remote_find_message(name);
    furi_check(message);

    infrared_worker_set_decoded_signal(app->ir_worker, message);
    infrared_worker_tx_start(app->ir_worker);
    infrared_worker_tx_stop(app->ir_worker);
}

static void samsung_remote_draw_home(Canvas* canvas, SamsungRemoteApp* app) {
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 12, "Samsung Remote");

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 8, 32, "Power");
    canvas_draw_str(canvas, 8, 45, "Simulate Remote");

    const uint8_t cursor_y = app->selected == SamsungRemoteHomePower ? 32 : 45;
    canvas_draw_str(canvas, 0, cursor_y, ">");
}

static void samsung_remote_draw_physical(Canvas* canvas) {
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 12, "Physical Remote");

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 27, "Arrows = TV nav");
    canvas_draw_str(canvas, 2, 38, "OK = Select");
    canvas_draw_str(canvas, 2, 49, "Back = Return");
    canvas_draw_str(canvas, 2, 60, "Hold Back = Home");
}

static void samsung_remote_draw_callback(Canvas* canvas, void* context) {
    SamsungRemoteApp* app = context;

    if(app->screen == SamsungRemoteScreenHome) {
        samsung_remote_draw_home(canvas, app);
    } else {
        samsung_remote_draw_physical(canvas);
    }
}

static void samsung_remote_input_callback(InputEvent* input_event, void* context) {
    FuriMessageQueue* event_queue = context;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

static void samsung_remote_handle_home_input(SamsungRemoteApp* app, const InputEvent* event) {
    if(event->type != InputTypeShort) {
        return;
    }

    if(event->key == InputKeyUp || event->key == InputKeyDown) {
        if(app->selected == SamsungRemoteHomePower) {
            app->selected = SamsungRemoteHomeSimulate;
        } else {
            app->selected = SamsungRemoteHomePower;
        }
    } else if(event->key == InputKeyOk) {
        if(app->selected == SamsungRemoteHomePower) {
            samsung_remote_send(app, "POWER");
        } else {
            app->screen = SamsungRemoteScreenPhysical;
        }
    } else if(event->key == InputKeyBack) {
        app->running = false;
    }
}

static void samsung_remote_handle_physical_input(SamsungRemoteApp* app, const InputEvent* event) {
    if(event->type == InputTypeLong && event->key == InputKeyBack) {
        app->screen = SamsungRemoteScreenHome;
        return;
    }

    if(event->type != InputTypeShort) {
        return;
    }

    if(event->key == InputKeyUp) {
        samsung_remote_send(app, "Up");
    } else if(event->key == InputKeyDown) {
        samsung_remote_send(app, "Down");
    } else if(event->key == InputKeyLeft) {
        samsung_remote_send(app, "Left");
    } else if(event->key == InputKeyRight) {
        samsung_remote_send(app, "Right");
    } else if(event->key == InputKeyOk) {
        samsung_remote_send(app, "Select");
    } else if(event->key == InputKeyBack) {
        samsung_remote_send(app, "Return");
    }
}

int32_t samsung_remote_app(void* p) {
    UNUSED(p);

    SamsungRemoteApp* app = malloc(sizeof(SamsungRemoteApp));
    app->event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    app->view_port = view_port_alloc();
    app->gui = furi_record_open(RECORD_GUI);
    app->ir_worker = infrared_worker_alloc();
    app->screen = SamsungRemoteScreenHome;
    app->selected = SamsungRemoteHomePower;
    app->running = true;

    infrared_worker_tx_set_get_signal_callback(
        app->ir_worker, infrared_worker_tx_get_signal_steady_callback, NULL);

    view_port_draw_callback_set(app->view_port, samsung_remote_draw_callback, app);
    view_port_input_callback_set(app->view_port, samsung_remote_input_callback, app->event_queue);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    InputEvent event;
    while(app->running) {
        if(furi_message_queue_get(app->event_queue, &event, FuriWaitForever) == FuriStatusOk) {
            if(app->screen == SamsungRemoteScreenHome) {
                samsung_remote_handle_home_input(app, &event);
            } else {
                samsung_remote_handle_physical_input(app, &event);
            }

            view_port_update(app->view_port);
        }
    }

    gui_remove_view_port(app->gui, app->view_port);
    infrared_worker_free(app->ir_worker);
    view_port_free(app->view_port);
    furi_record_close(RECORD_GUI);
    furi_message_queue_free(app->event_queue);
    free(app);

    return 0;
}
