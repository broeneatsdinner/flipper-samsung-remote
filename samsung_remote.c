#include <furi.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <input/input.h>
#include <infrared.h>
#include <infrared_worker.h>
#include <stdlib.h>
#include <string.h>

#define SAMSUNG_REMOTE_BACK_HOLD_MS 3000
#define SAMSUNG_REMOTE_VOLUME_HOLD_MS 2000
#define SAMSUNG_REMOTE_VOLUME_REPEAT_MS 200
#define SAMSUNG_REMOTE_TX_GAP_MS 150
#define SAMSUNG_REMOTE_TX_QUEUE_SIZE 8

typedef enum {
    SamsungRemoteScreenHome,
    SamsungRemoteScreenPhysical,
} SamsungRemoteScreen;

typedef enum {
    SamsungRemoteHomePower,
    SamsungRemoteHomeSimulate,
} SamsungRemoteHomeItem;

typedef enum {
    SamsungRemoteVolumeButtonNone,
    SamsungRemoteVolumeButtonUp,
    SamsungRemoteVolumeButtonDown,
} SamsungRemoteVolumeButton;

typedef struct {
    FuriMessageQueue* event_queue;
    ViewPort* view_port;
    Gui* gui;
    InfraredWorker* ir_worker;
    SamsungRemoteScreen screen;
    SamsungRemoteHomeItem selected;
    uint32_t back_press_tick;
    bool back_pressed;
    bool back_hold_handled;
    SamsungRemoteVolumeButton volume_button;
    uint32_t volume_press_tick;
    uint32_t volume_last_repeat_tick;
    bool volume_repeat_active;
    const char* tx_queue[SAMSUNG_REMOTE_TX_QUEUE_SIZE];
    uint8_t tx_queue_head;
    uint8_t tx_queue_tail;
    uint8_t tx_queue_count;
    uint32_t tx_last_tick;
    bool tx_sent_once;
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
    {"VOL+", {InfraredProtocolSamsung32, 0x00000007, 0x00000007, false}},
    {"VOL-", {InfraredProtocolSamsung32, 0x00000007, 0x0000000B, false}},
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

static void samsung_remote_reset_tx_queue(SamsungRemoteApp* app) {
    app->tx_queue_head = 0;
    app->tx_queue_tail = 0;
    app->tx_queue_count = 0;
    app->tx_last_tick = 0;
    app->tx_sent_once = false;
}

static void samsung_remote_enqueue_send(SamsungRemoteApp* app, const char* name) {
    furi_check(samsung_remote_find_message(name));

    if(app->tx_queue_count >= SAMSUNG_REMOTE_TX_QUEUE_SIZE) {
        return;
    }

    app->tx_queue[app->tx_queue_tail] = name;
    app->tx_queue_tail = (app->tx_queue_tail + 1) % SAMSUNG_REMOTE_TX_QUEUE_SIZE;
    app->tx_queue_count++;
}

static const char* samsung_remote_tx_queue_pop(SamsungRemoteApp* app) {
    furi_check(app->tx_queue_count > 0);

    const char* name = app->tx_queue[app->tx_queue_head];
    app->tx_queue_head = (app->tx_queue_head + 1) % SAMSUNG_REMOTE_TX_QUEUE_SIZE;
    app->tx_queue_count--;
    return name;
}

static void samsung_remote_reset_volume_hold(SamsungRemoteApp* app) {
    app->volume_button = SamsungRemoteVolumeButtonNone;
    app->volume_press_tick = 0;
    app->volume_last_repeat_tick = 0;
    app->volume_repeat_active = false;
}

static void samsung_remote_set_screen(SamsungRemoteApp* app, SamsungRemoteScreen screen) {
    app->screen = screen;
    view_port_set_orientation(
        app->view_port,
        screen == SamsungRemoteScreenPhysical ? ViewPortOrientationVertical :
                                                ViewPortOrientationHorizontal);
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
    canvas_draw_str(canvas, 2, 12, "Samsung");
    canvas_draw_str(canvas, 2, 24, "Remote");

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 42, "Arrows");
    canvas_draw_str(canvas, 8, 53, "= TV nav");
    canvas_draw_str(canvas, 2, 66, "OK");
    canvas_draw_str(canvas, 8, 77, "= Select");
    canvas_draw_str(canvas, 2, 90, "Hold U/D");
    canvas_draw_str(canvas, 8, 101, "= Vol");
    canvas_draw_str(canvas, 2, 114, "Back");
    canvas_draw_str(canvas, 2, 125, "= Ret/Home");
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
            samsung_remote_enqueue_send(app, "POWER");
        } else {
            app->back_pressed = false;
            app->back_hold_handled = false;
            samsung_remote_reset_volume_hold(app);
            samsung_remote_set_screen(app, SamsungRemoteScreenPhysical);
        }
    } else if(event->key == InputKeyBack) {
        app->running = false;
    }
}

static void samsung_remote_handle_physical_input(SamsungRemoteApp* app, const InputEvent* event) {
    if((event->key == InputKeyUp || event->key == InputKeyDown) &&
       event->type == InputTypePress) {
        app->volume_button = event->key == InputKeyUp ? SamsungRemoteVolumeButtonUp :
                                                       SamsungRemoteVolumeButtonDown;
        app->volume_press_tick = furi_get_tick();
        app->volume_last_repeat_tick = 0;
        app->volume_repeat_active = false;
        return;
    }

    if((event->key == InputKeyUp || event->key == InputKeyDown) &&
       event->type == InputTypeRelease) {
        const bool up_released =
            event->key == InputKeyUp && app->volume_button == SamsungRemoteVolumeButtonUp;
        const bool down_released =
            event->key == InputKeyDown && app->volume_button == SamsungRemoteVolumeButtonDown;

        if(!up_released && !down_released) {
            return;
        }

        const uint32_t held_ticks = furi_get_tick() - app->volume_press_tick;
        const bool send_nav =
            !app->volume_repeat_active &&
            held_ticks < furi_ms_to_ticks(SAMSUNG_REMOTE_VOLUME_HOLD_MS);
        const bool send_volume =
            !app->volume_repeat_active &&
            held_ticks >= furi_ms_to_ticks(SAMSUNG_REMOTE_VOLUME_HOLD_MS);
        samsung_remote_reset_volume_hold(app);

        if(send_nav) {
            samsung_remote_enqueue_send(app, event->key == InputKeyUp ? "Up" : "Down");
        } else if(send_volume) {
            samsung_remote_enqueue_send(app, event->key == InputKeyUp ? "VOL+" : "VOL-");
        }

        return;
    }

    if(event->key == InputKeyBack && event->type == InputTypePress) {
        app->back_press_tick = furi_get_tick();
        app->back_pressed = true;
        app->back_hold_handled = false;
        return;
    }

    if(event->key == InputKeyBack && event->type == InputTypeRelease) {
        if(!app->back_pressed) {
            return;
        }

        const bool send_return = !app->back_hold_handled;
        app->back_pressed = false;
        app->back_hold_handled = false;

        if(send_return) {
            samsung_remote_enqueue_send(app, "Return");
        }

        return;
    }

    if(event->type != InputTypeShort) {
        return;
    }

    if(event->key == InputKeyUp || event->key == InputKeyDown) {
        return;
    } else if(event->key == InputKeyLeft) {
        samsung_remote_enqueue_send(app, "Left");
    } else if(event->key == InputKeyRight) {
        samsung_remote_enqueue_send(app, "Right");
    } else if(event->key == InputKeyOk) {
        samsung_remote_enqueue_send(app, "Select");
    }
}

static void samsung_remote_handle_back_hold(SamsungRemoteApp* app) {
    if(app->screen != SamsungRemoteScreenPhysical || !app->back_pressed || app->back_hold_handled) {
        return;
    }

    const uint32_t held_ticks = furi_get_tick() - app->back_press_tick;
    if(held_ticks >= furi_ms_to_ticks(SAMSUNG_REMOTE_BACK_HOLD_MS)) {
        app->back_hold_handled = true;
        samsung_remote_reset_volume_hold(app);
        samsung_remote_set_screen(app, SamsungRemoteScreenHome);
        view_port_update(app->view_port);
    }
}

static void samsung_remote_handle_volume_hold(SamsungRemoteApp* app) {
    if(app->screen != SamsungRemoteScreenPhysical ||
       app->volume_button == SamsungRemoteVolumeButtonNone) {
        return;
    }

    const uint32_t now = furi_get_tick();
    const char* command = app->volume_button == SamsungRemoteVolumeButtonUp ? "VOL+" : "VOL-";

    if(!app->volume_repeat_active) {
        const uint32_t held_ticks = now - app->volume_press_tick;
        if(held_ticks < furi_ms_to_ticks(SAMSUNG_REMOTE_VOLUME_HOLD_MS)) {
            return;
        }

        app->volume_repeat_active = true;
        app->volume_last_repeat_tick = now;
        samsung_remote_enqueue_send(app, command);
        return;
    }

    const uint32_t repeat_ticks = furi_ms_to_ticks(SAMSUNG_REMOTE_VOLUME_REPEAT_MS);
    if(now - app->volume_last_repeat_tick >= repeat_ticks) {
        app->volume_last_repeat_tick = now;
        samsung_remote_enqueue_send(app, command);
    }
}

static void samsung_remote_handle_tx_queue(SamsungRemoteApp* app) {
    if(app->tx_queue_count == 0) {
        return;
    }

    const uint32_t now = furi_get_tick();
    if(app->tx_sent_once) {
        const uint32_t elapsed_ticks = now - app->tx_last_tick;
        if(elapsed_ticks < furi_ms_to_ticks(SAMSUNG_REMOTE_TX_GAP_MS)) {
            return;
        }
    }

    const char* command = samsung_remote_tx_queue_pop(app);
    samsung_remote_send(app, command);
    app->tx_last_tick = furi_get_tick();
    app->tx_sent_once = true;
}

static uint32_t samsung_remote_back_timeout(SamsungRemoteApp* app) {
    if(app->screen != SamsungRemoteScreenPhysical || !app->back_pressed || app->back_hold_handled) {
        return FuriWaitForever;
    }

    const uint32_t held_ticks = furi_get_tick() - app->back_press_tick;
    const uint32_t hold_ticks = furi_ms_to_ticks(SAMSUNG_REMOTE_BACK_HOLD_MS);

    if(held_ticks >= hold_ticks) {
        return 0;
    }

    return hold_ticks - held_ticks;
}

static uint32_t samsung_remote_volume_timeout(SamsungRemoteApp* app) {
    if(app->screen != SamsungRemoteScreenPhysical ||
       app->volume_button == SamsungRemoteVolumeButtonNone) {
        return FuriWaitForever;
    }

    const uint32_t now = furi_get_tick();

    if(!app->volume_repeat_active) {
        const uint32_t held_ticks = now - app->volume_press_tick;
        const uint32_t hold_ticks = furi_ms_to_ticks(SAMSUNG_REMOTE_VOLUME_HOLD_MS);

        if(held_ticks >= hold_ticks) {
            return 0;
        }

        return hold_ticks - held_ticks;
    }

    const uint32_t repeat_ticks = furi_ms_to_ticks(SAMSUNG_REMOTE_VOLUME_REPEAT_MS);
    const uint32_t elapsed_ticks = now - app->volume_last_repeat_tick;

    if(elapsed_ticks >= repeat_ticks) {
        return 0;
    }

    return repeat_ticks - elapsed_ticks;
}

static uint32_t samsung_remote_tx_timeout(SamsungRemoteApp* app) {
    if(app->tx_queue_count == 0) {
        return FuriWaitForever;
    }

    if(!app->tx_sent_once) {
        return 0;
    }

    const uint32_t now = furi_get_tick();
    const uint32_t gap_ticks = furi_ms_to_ticks(SAMSUNG_REMOTE_TX_GAP_MS);
    const uint32_t elapsed_ticks = now - app->tx_last_tick;

    if(elapsed_ticks >= gap_ticks) {
        return 0;
    }

    return gap_ticks - elapsed_ticks;
}

static uint32_t samsung_remote_timeout_min(uint32_t left, uint32_t right) {
    if(left == FuriWaitForever) {
        return right;
    }

    if(right == FuriWaitForever) {
        return left;
    }

    return left < right ? left : right;
}

static uint32_t samsung_remote_event_timeout(SamsungRemoteApp* app) {
    return samsung_remote_timeout_min(
        samsung_remote_timeout_min(
            samsung_remote_back_timeout(app), samsung_remote_volume_timeout(app)),
        samsung_remote_tx_timeout(app));
}

static void samsung_remote_handle_timeouts(SamsungRemoteApp* app) {
    samsung_remote_handle_back_hold(app);
    samsung_remote_handle_volume_hold(app);
    samsung_remote_handle_tx_queue(app);
}

static bool samsung_remote_consume_handled_back_event(
    SamsungRemoteApp* app,
    const InputEvent* event) {
    if(!app->back_hold_handled || event->key != InputKeyBack) {
        return false;
    }

    if(event->type == InputTypePress) {
        app->back_hold_handled = false;
        return false;
    }

    if(event->type == InputTypeRelease) {
        app->back_pressed = false;
    }

    return true;
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
    app->back_press_tick = 0;
    app->back_pressed = false;
    app->back_hold_handled = false;
    samsung_remote_reset_volume_hold(app);
    samsung_remote_reset_tx_queue(app);
    app->running = true;

    infrared_worker_tx_set_get_signal_callback(
        app->ir_worker, infrared_worker_tx_get_signal_steady_callback, NULL);

    view_port_draw_callback_set(app->view_port, samsung_remote_draw_callback, app);
    view_port_input_callback_set(app->view_port, samsung_remote_input_callback, app->event_queue);
    view_port_set_orientation(app->view_port, ViewPortOrientationHorizontal);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    InputEvent event;
    while(app->running) {
        const uint32_t event_timeout = samsung_remote_event_timeout(app);
        if(furi_message_queue_get(app->event_queue, &event, event_timeout) == FuriStatusOk) {
            if(!samsung_remote_consume_handled_back_event(app, &event)) {
                if(app->screen == SamsungRemoteScreenHome) {
                    samsung_remote_handle_home_input(app, &event);
                } else {
                    samsung_remote_handle_physical_input(app, &event);
                }
            }

            view_port_update(app->view_port);
        } else {
            samsung_remote_handle_timeouts(app);
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
