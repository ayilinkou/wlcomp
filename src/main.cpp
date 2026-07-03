#include <cstdio>
#include <cstdlib>
#include <format>
#include <iostream>
#include <wayland-protocols/xdg-shell-enum.h>
#include <wayland-server-core.h>
#include <wayland-util.h>

#include "wlroots.hpp"

struct Server
{
    wl_display* display = nullptr;
    wlr_backend* backend = nullptr;
    wlr_renderer* renderer = nullptr;
    wlr_allocator* allocator = nullptr;
    wlr_compositor* compositor = nullptr;
    wlr_output_layout* output_layout = nullptr;
    wlr_scene* scene = nullptr;
    wlr_scene_output_layout* scene_output_layout = nullptr;
    wlr_xdg_shell* xdg_shell = nullptr;
    wlr_seat* seat = nullptr;

    wl_listener new_output_listener = {};
    wl_listener new_xdg_toplevel_listener = {};
};

struct Toplevel
{
    wlr_xdg_toplevel* xdg_toplevel = nullptr;
    wlr_scene_tree* scene_tree = nullptr;

    wl_listener map_listener = {};
    wl_listener unmap_listener = {};
    wl_listener destroy_listener = {};
};

// called whenever the backend detects a new output
static void handle_new_output(wl_listener* listener, void* data)
{
    Server* server = wl_container_of(listener, server, new_output_listener);
    wlr_output* wlr_output_ptr = static_cast<wlr_output*>(data);

    wlr_output_init_render(wlr_output_ptr, server->allocator, server->renderer);

    wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);

    wlr_output_mode* mode = wlr_output_preferred_mode(wlr_output_ptr);
    if (mode != nullptr)
    {
        wlr_output_state_set_mode(&state, mode);
    }

    wlr_output_commit_state(wlr_output_ptr, &state);
    wlr_output_state_finish(&state);

    // register the output in the layout at 0, 0
    wlr_output_layout_output* layout_output =
        wlr_output_layout_add_auto(server->output_layout, wlr_output_ptr);

    wlr_scene_output* scene_output =
        wlr_scene_output_create(server->scene, wlr_output_ptr);
    wlr_scene_output_layout_add_output(server->scene_output_layout,
                                       layout_output, scene_output);

    auto msg = std::format("New output: {} ({}x{})\n", wlr_output_ptr->name,
                           wlr_output_ptr->width, wlr_output_ptr->height);
    std::cout << msg;
}

static void handle_toplevel_map(wl_listener* listener, void* data)
{
    Toplevel* toplevel = wl_container_of(listener, toplevel, map_listener);
    std::cout << "Toplevel mapped\n";
    // Content now visible — nothing else needed here yet, the scene_tree
    // was already added to the graph back in handle_new_xdg_toplevel.
}

static void handle_toplevel_unmap(wl_listener* listener, void* data)
{
    Toplevel* toplevel = wl_container_of(listener, toplevel, unmap_listener);
    std::cout << "Toplevel unmapped\n";
    // Surface no longer visible — wlr_scene handles hiding it once its
    // backing surface is unmapped, so nothing to do manually here either.
}

static void handle_toplevel_destroy(wl_listener* listener, void* data)
{
    Toplevel* toplevel = wl_container_of(listener, toplevel, destroy_listener);
    std::cout << "Toplevel destroyed\n";
    delete toplevel;
}

// called whenever an application wants a window
static void handle_new_xdg_toplevel(wl_listener* listener, void* data)
{
    Server* server =
        wl_container_of(listener, server, new_xdg_toplevel_listener);
    wlr_xdg_toplevel* xdg_toplevel = static_cast<wlr_xdg_toplevel*>(data);

    std::cout << std::format(
        "New xdg toplevel: title=\"{}\" app_id=\"{}\"\n",
        xdg_toplevel->title ? xdg_toplevel->title : "(none)",
        xdg_toplevel->app_id ? xdg_toplevel->app_id : "(none)");

    Toplevel* toplevel = new Toplevel();
    toplevel->xdg_toplevel = xdg_toplevel;
    toplevel->scene_tree =
        wlr_scene_xdg_surface_create(&server->scene->tree, xdg_toplevel->base);

    toplevel->map_listener.notify = handle_toplevel_map;
    wl_signal_add(&xdg_toplevel->base->surface->events.map,
                  &toplevel->map_listener);

    toplevel->unmap_listener.notify = handle_toplevel_unmap;
    wl_signal_add(&xdg_toplevel->base->surface->events.unmap,
                  &toplevel->unmap_listener);

    toplevel->destroy_listener.notify = handle_toplevel_destroy;
    wl_signal_add(&xdg_toplevel->events.destroy, &toplevel->destroy_listener);
}

int main()
{
    wlr_log_init(WLR_DEBUG, nullptr);

    Server server{};

    // create display
    server.display = wl_display_create();
    if (server.display == nullptr)
    {
        std::cerr << "Failed to create a wl_display!\n";
        return EXIT_FAILURE;
    }

    // autocreate backend
    server.backend = wlr_backend_autocreate(
        wl_display_get_event_loop(server.display), nullptr);
    if (server.backend == nullptr)
    {
        std::cerr << "Failed to create a wlr_backend!\n";
        return EXIT_FAILURE;
    }

    // autocreate renderer
    server.renderer = wlr_renderer_autocreate(server.backend);
    if (server.renderer == nullptr)
    {
        std::cerr << "Failed to create renderer!\n";
        return EXIT_FAILURE;
    }
    wlr_renderer_init_wl_display(server.renderer, server.display);

    // autocreate allocator
    server.allocator =
        wlr_allocator_autocreate(server.backend, server.renderer);
    if (server.allocator == nullptr)
    {
        std::cerr << "Failed to create allocator!\n";
        return EXIT_FAILURE;
    }

    // create compositor
    uint32_t version = 6u;
    server.compositor =
        wlr_compositor_create(server.display, version, server.renderer);
    wlr_subcompositor_create(server.display);
    wlr_data_device_manager_create(server.display);

    server.output_layout = wlr_output_layout_create(server.display);

    // create scene graph
    server.scene = wlr_scene_create();
    server.scene_output_layout =
        wlr_scene_attach_output_layout(server.scene, server.output_layout);

    server.new_output_listener.notify = handle_new_output;
    wl_signal_add(&server.backend->events.new_output,
                  &server.new_output_listener);

    uint32_t shell_protocol_version = 3u;
    server.xdg_shell =
        wlr_xdg_shell_create(server.display, shell_protocol_version);
    server.new_xdg_toplevel_listener.notify = handle_new_xdg_toplevel;
    wl_signal_add(&server.xdg_shell->events.new_toplevel,
                  &server.new_xdg_toplevel_listener);

    // TODO: add listeners
    server.seat = wlr_seat_create(server.display, "seat0");

    const char* socket = wl_display_add_socket_auto(server.display);
    if (socket == nullptr)
    {
        std::cerr << "Failed to open a wayland socket!\n";
        wlr_backend_destroy(server.backend);
        return EXIT_FAILURE;
    }

    if (!wlr_backend_start(server.backend))
    {
        std::cerr << "Failed to start backend!\n";
        wlr_backend_destroy(server.backend);
        wl_display_destroy(server.display);
        return EXIT_FAILURE;
    }

    setenv("WAYLAND_DISPLAY", socket, true);
    std::cout << std::format("Running compositor on WAYLAND_DISPLAY={}\n",
                             socket);

    // run event loop
    wl_display_run(server.display);

    wl_display_destroy_clients(server.display);
    wl_display_destroy(server.display);

    return EXIT_SUCCESS;
}
