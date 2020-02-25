/* xyes
 *
 * Copyright © 2020 Sergey Bugaev <bugaevc@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

struct context {
    Display *display;
    int xfd;
    Atom wm_delete_window;
    Window window;
    GC gc;
    int window_height;
    int font_height;
    int offset;
    const char *message;
    int should_quit;
};

static void draw(struct context *context) {
    XClearWindow(context->display, context->window);
    for (
        int y = context->offset;
        y < context->window_height;
        y += context->font_height
    ) {
        XDrawString(
            context->display, context->window, context->gc,
            10, y,
            context->message, strlen(context->message)
        );
    }
}

static void setup(struct context *context, int argc, char *argv[]) {
    switch (argc) {
    case 0:
        fprintf(stderr, "Empty argv\n");
        exit(1);
        break;
    case 1:
        context->message = "y";
        break;
    case 2:
        context->message = argv[1];
        break;
    default:
        fprintf(stderr, "Usage: %s [string]\n", argv[0]);
        exit(1);
        break;
    }

    context->display = XOpenDisplay(NULL);
    if (context->display == NULL) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }
    context->xfd = ConnectionNumber(context->display);

    int screen = DefaultScreen(context->display);
    context->gc = DefaultGC(context->display, screen);

    XFontStruct *default_font = XQueryFont(context->display, XGContextFromGC(context->gc));
    context->font_height = default_font->ascent + default_font->descent;

    int window_width = XTextWidth(default_font, context->message, strlen(context->message)) + 100;

    Window root_window = RootWindow(context->display, screen);
    context->window = XCreateSimpleWindow(
        context->display, root_window,
        100, 100, window_width, 300,
        1, BlackPixel(context->display, screen), WhitePixel(context->display, screen)
    );
    XSelectInput(context->display, context->window, ExposureMask | StructureNotifyMask);
    context->window_height = 300;

    XTextProperty name_property;
    char *l[] = { "xyes" };
    XStringListToTextProperty(l, 1, &name_property);
    XSetWMProperties(
        context->display, context->window,
        &name_property, NULL,
        argv, argc,
        NULL, NULL, NULL
    );
    context->wm_delete_window = XInternAtom(context->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(context->display, context->window, &context->wm_delete_window, 1);

    XMapWindow(context->display, context->window);

    context->offset = 0;
    context->should_quit = 0;
}

static void handle_event(struct context *context, const XEvent *event) {
    switch (event->type) {
    case Expose:
        draw(context);
        break;
    case ClientMessage:
        if (event->xclient.data.l[0] == context->wm_delete_window) {
            context->should_quit = 1;
        }
        break;
    case ConfigureNotify:
        context->window_height = event->xconfigure.height;
        break;
    }
}

int main(int argc, char *argv[]) {
    struct context context;

    setup(&context, argc, argv);

    while (!context.should_quit) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(context.xfd, &fds);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 16666;

        int can_read = select(context.xfd + 1, &fds, NULL, NULL, &tv);
        if (can_read < 0) {
            perror("select");
            return 1;
        }

        if (can_read == 0) {
            // Timed out.
            context.offset -= 3;
            context.offset %= context.font_height;
            draw(&context);
        }

        while (XEventsQueued(context.display, QueuedAlready)) {
            XEvent event;
            XNextEvent(context.display, &event);
            handle_event(&context, &event);
        }

        XFlush(context.display);
    }

    XCloseDisplay(context.display);
    return 0;
}
