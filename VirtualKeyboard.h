#ifndef VIRTUALKEYBOARD_H
#define VIRTUALKEYBOARD_H

#include <QGridLayout>
#include <QMouseEvent>
#include <QPoint>
#include <QPushButton>
#include <QWidget>
#include <linux/uinput.h>
#include <QMap>
#include <fcntl.h>

// Wayland headers
#include <wayland-client.h>
#include <wayland-virtual-keyboard-unstable-v1-client-protocol.h>
#include <wayland-input-method-unstable-v1-client-protocol.h>

// X11 headers
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

class VirtualKeyboard : public QWidget {
  Q_OBJECT

public:
  VirtualKeyboard(QWidget *parent = nullptr);
  ~VirtualKeyboard();

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void showEvent(QShowEvent *event) override;

private:
  int m_uinputFd = -1;
  void initUinput();
  void setupUI();
  void sendKey(int keycode, bool pressed);
  void releaseAllKeys();
  void updateKeyCaps();

  // UI Helpers
  QPushButton *createKey(int keycode, const QString &label, const QString &shiftLabel = QString(), float stretch = 1.0);
  QWidget* m_dragHandle;

  // Platform state
  bool isWayland;
  QPoint m_dragPosition;
  bool m_isResizing = false;

  struct KeyInfo {
      QString normal;
      QString shift;
  };
  QMap<int, QPushButton*> m_buttons;
  QMap<QPushButton*, KeyInfo> m_keyMap;

  // Wayland specific members
  wl_display *m_display = nullptr;
  wl_seat *m_seat = nullptr;
  zwp_virtual_keyboard_v1 *m_virtualKeyboard = nullptr;
  zwp_virtual_keyboard_manager_v1 *m_manager = nullptr;

  zwp_input_panel_v1 *m_imManager = nullptr; // The 'Manager' in v1 is called Input Panel
  zwp_input_method_v1 *m_inputMethod = nullptr;
  zwp_input_method_context_v1 *m_inputContext = nullptr; // You need this for sending keys
  uint32_t m_serial = 0; // Required for input method synchronization

  // X11 specific members
  Display *m_xDisplay = nullptr;

  // Modifiers state
  bool m_shiftActive = false;
  bool m_altActive = false;
  bool m_metaActive = false;
  bool m_capsActive = false;
};

#endif
