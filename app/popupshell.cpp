#include "popupshell.h"

#include "inputmethod.h"

#include <QWindow>
#include <QtWaylandClient/private/qwaylandwindow_p.h>

using namespace QtWaylandClient;

PopupShellIntegration::PopupShellIntegration(InputMethod *inputMethod)
    : m_inputMethod(inputMethod)
{
}

QWaylandShellSurface *PopupShellIntegration::createShellSurface(QWaylandWindow *window)
{
    return new PopupShellSurface(window, m_inputMethod->get_input_popup_surface(window->wlSurface()));
}

void PopupShellIntegration::attach(QWindow *window, InputMethod *inputMethod)
{
    window->create();
    auto *waylandWindow = dynamic_cast<QWaylandWindow *>(window->handle());
    if (!waylandWindow)
        qFatal("glyphpop: the picker needs the Wayland platform");
    static PopupShellIntegration integration(inputMethod);
    waylandWindow->setShellIntegration(&integration);
}

PopupShellSurface::PopupShellSurface(QWaylandWindow *window, struct ::zwp_input_popup_surface_v2 *popup)
    : QWaylandShellSurface(window)
    , QtWayland::zwp_input_popup_surface_v2(popup)
{
}

PopupShellSurface::~PopupShellSurface()
{
    destroy();
}
