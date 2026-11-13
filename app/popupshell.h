#pragma once

#include <QtWaylandClient/private/qwaylandshellintegration_p.h>
#include <QtWaylandClient/private/qwaylandshellsurface_p.h>

#include "qwayland-input-method-unstable-v2.h"

class InputMethod;
class QWindow;

// Gives a QWindow the input-method popup role, so the compositor places it at the text caret.
class PopupShellIntegration : public QtWaylandClient::QWaylandShellIntegration
{
public:
    explicit PopupShellIntegration(InputMethod *inputMethod);

    bool initialize(QtWaylandClient::QWaylandDisplay *) override { return true; }
    QtWaylandClient::QWaylandShellSurface *createShellSurface(QtWaylandClient::QWaylandWindow *window) override;

    // Call once before the window is first shown.
    static void attach(QWindow *window, InputMethod *inputMethod);

private:
    InputMethod *m_inputMethod;
};

class PopupShellSurface : public QtWaylandClient::QWaylandShellSurface, public QtWayland::zwp_input_popup_surface_v2
{
    Q_OBJECT

public:
    PopupShellSurface(QtWaylandClient::QWaylandWindow *window, struct ::zwp_input_popup_surface_v2 *popup);
    ~PopupShellSurface() override;
};
