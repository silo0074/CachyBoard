#include "VirtualKeyboard.h"
#include <LayerShellQt/Window>
#include <QDebug>
#include <QGuiApplication>
#include <QStyle>
#include <QVBoxLayout>
#include <QLabel>
#include <linux/input-event-codes.h>
#include <unistd.h>
#include <linux/input.h>

extern "C" {
    #include "wayland-virtual-keyboard-unstable-v1-client-protocol.h"
    #include "wayland-input-method-unstable-v1-client-protocol.h"
}

VirtualKeyboard::VirtualKeyboard(QWidget *parent) : QWidget(parent) {
  connect(this, &QWidget::destroyed, qApp, &QCoreApplication::quit);

  // Basic setup
  setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::Tool);
  setAttribute(Qt::WA_TranslucentBackground);
  
  // Check platform
  isWayland = QGuiApplication::platformName().contains("wayland");

  if (isWayland) {
    
    qDebug() << "Wayland detected.";

    m_uinputFd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (m_uinputFd >= 0) {
        ioctl(m_uinputFd, UI_SET_EVBIT, EV_KEY);
        // Enable all keys (you can loop 1-255)
        for(int i = 1; i < 255; i++) ioctl(m_uinputFd, UI_SET_KEYBIT, i);

        struct uinput_setup usetup;
        memset(&usetup, 0, sizeof(usetup));
        usetup.id.bustype = BUS_USB;
        usetup.id.vendor  = 0x1234; 
        usetup.id.product = 0x5678;
        strcpy(usetup.name, "Custom Virtual Keyboard");

        ioctl(m_uinputFd, UI_DEV_SETUP, &usetup);
        ioctl(m_uinputFd, UI_DEV_CREATE);
        qDebug() << "uinput device created successfully!";
    } else {
        qDebug() << "Failed to open /dev/uinput:" << strerror(errno);
    }

    m_display = wl_display_connect(NULL);

    if (m_display) {
      qDebug() << "m_display is true";
      wl_registry *registry = wl_display_get_registry(m_display);

      static const wl_registry_listener registry_listener = {
          [](void *data, wl_registry *reg, uint32_t id, const char *interface, uint32_t version) {
              auto self = static_cast<VirtualKeyboard *>(data);
            //   qDebug() << "Available Interface:" << interface << "Version:" << version;

              if (strcmp(interface, "zwp_input_panel_v1") == 0) {
                  self->m_imManager = (zwp_input_panel_v1*)wl_registry_bind(reg, id, &zwp_input_panel_v1_interface, 1);
              } else if (strcmp(interface, "zwp_input_method_v1") == 0) {
                  self->m_inputMethod = (zwp_input_method_v1*)wl_registry_bind(reg, id, &zwp_input_method_v1_interface, 1);
                  
                  // Set up a listener to get the "context" when a text field is activated
                  static const zwp_input_method_v1_listener im_listener = {
                      [](void *data, zwp_input_method_v1 *, zwp_input_method_context_v1 *id) {
                          auto s = static_cast<VirtualKeyboard*>(data);
                          s->m_inputContext = id; // Store the context needed to send keys
                      },
                      [](void *data, zwp_input_method_v1 *, zwp_input_method_context_v1 *context) {
                          auto s = static_cast<VirtualKeyboard*>(data);
                          if (s->m_inputContext == context) s->m_inputContext = nullptr;
                      }
                  };
                  zwp_input_method_v1_add_listener(self->m_inputMethod, &im_listener, self);
              }
          },
          [](void *, wl_registry *, uint32_t) {}
      };

      wl_registry_add_listener(registry, &registry_listener, this); //
      wl_display_roundtrip(m_display);
      wl_display_roundtrip(m_display); // First: find interfaces
      wl_display_roundtrip(m_display); // Second: complete binding

      if (m_manager && m_seat) {
          qDebug() << "Success: Manager and Seat found.";
          m_virtualKeyboard = zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(m_manager, m_seat);
          wl_display_roundtrip(m_display); // Sync the creation
      } else if (m_imManager && m_seat) {
          qDebug() << "Falling back to Input Method Manager.";
          // m_inputMethod = zwp_input_method_manager_v1_get_input_method(m_imManager, m_seat);
          wl_display_roundtrip(m_display);
      } else {
          if (!m_manager) qDebug() << "Error: zwp_virtual_keyboard_manager_v1 NOT found in registry!";
          if (!m_seat)    qDebug() << "Error: wl_seat NOT found in registry!";
      }

      if (m_virtualKeyboard) {
          qDebug() << "m_virtualKeyboard";
          // You must send a keymap, even if it's empty/null, 
          // to "activate" the virtual keyboard in many compositors.
          // This uses format 1 (libxkbcommon) and an empty file descriptor.
          zwp_virtual_keyboard_v1_keymap(m_virtualKeyboard, 1, -1, 0);
      }
    }
  } else {
    m_xDisplay = XOpenDisplay(NULL); //
  }

    initUinput();
    setupUI();
    resize(800, 300);

    setStyleSheet(
        "QWidget { background-color: rgba(35, 35, 35, 240); border-radius: 8px; }"
        "QPushButton { background-color: #444; color: white; border: 1px solid #555; "
        "border-radius: 4px; padding: 5px; font-size: 13px; font-weight: bold; min-width: 30px; }"
        "QPushButton:pressed { background-color: #0078d7; }"
        "QPushButton[active='true'] { background-color: #005a9e; border: 1px solid #00a2ff; }");
}

VirtualKeyboard::~VirtualKeyboard() {
    qDebug() << "Running destructor";
    if (m_uinputFd >= 0) {
        // Force release all keys before destroying the device
        releaseAllKeys(); 

        ioctl(m_uinputFd, UI_DEV_DESTROY, 0);
        
        // Use :: to specify the global Linux close() function
        ::close(m_uinputFd); 
        
        m_uinputFd = -1;
        qDebug() << "uinput device cleaned up successfully.";
    }

    if (m_virtualKeyboard) zwp_virtual_keyboard_v1_destroy(m_virtualKeyboard);
    if (m_inputMethod) zwp_input_method_v1_destroy(m_inputMethod);
    if (m_display) wl_display_disconnect(m_display);
    if (m_xDisplay) XCloseDisplay(m_xDisplay);
}


void VirtualKeyboard::releaseAllKeys() {
    if (m_uinputFd < 0) return;
    
    // Scan through standard keycodes and force release
    for (int i = 1; i < 248; i++) {
        struct input_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = EV_KEY;
        ev.code = i;
        ev.value = 0; // Release
        write(m_uinputFd, &ev, sizeof(ev));
    }
    
    // Sync the report
    struct input_event syn;
    memset(&syn, 0, sizeof(syn));
    syn.type = EV_SYN;
    syn.code = SYN_REPORT;
    syn.value = 0;
    write(m_uinputFd, &syn, sizeof(syn));
}


void VirtualKeyboard::initUinput() {
    m_uinputFd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (m_uinputFd < 0) {
        qDebug() << "CRITICAL: Could not open /dev/uinput:" << strerror(errno);
        return;
    }

    // 1. Enable Key Events
    ioctl(m_uinputFd, UI_SET_EVBIT, EV_KEY);
    
    // 2. Enable ALL keys (0 to 255 covers all standard Linux keycodes)
    for (int i = 0; i < 255; i++) {
        ioctl(m_uinputFd, UI_SET_KEYBIT, i);
    }

    // 3. Setup Device Info
    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor  = 0x1234; // Dummy IDs
    usetup.id.product = 0x5678;
    strcpy(usetup.name, "Qt Virtual Keyboard Device");

    ioctl(m_uinputFd, UI_DEV_SETUP, &usetup);
    ioctl(m_uinputFd, UI_DEV_CREATE);
    
    qDebug() << "Uinput device 'Qt Virtual Keyboard Device' created!";
}


void VirtualKeyboard::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    
    if (windowHandle()) {
        if (auto lsw = LayerShellQt::Window::get(windowHandle())) {
            lsw->setLayer(LayerShellQt::Window::LayerOverlay);
            
            // CRITICAL: InteractivityNone allows clicks to pass THROUGH the 
            // empty areas of the keyboard to the app behind it.
            lsw->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
            
            // Anchor to Bottom/Left/Right only. 
            // Do NOT anchor to Top, or it will stretch to full screen.
            lsw->setAnchors(LayerShellQt::Window::Anchors(
              LayerShellQt::Window::AnchorBottom | 
              LayerShellQt::Window::AnchorLeft | 
              LayerShellQt::Window::AnchorRight
          ));
            
            // Set an exclusive zone of 0 so it doesn't push other windows up,
            // or a positive value equal to height if you want it to push apps up.
            lsw->setExclusiveZone(0); 
            
            // Force the height so it doesn't fill the screen
            resize(QGuiApplication::primaryScreen()->size().width() * 0.8, 300);
        }
    }
    updateKeyCaps();
}


void VirtualKeyboard::setupUI() {
    // Master Vertical Layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Drag Handle (The Top Bar)
    QWidget *handle = new QWidget(this);
    handle->setFixedHeight(25);
    handle->setStyleSheet("background-color: #333; border-top-left-radius: 8px; border-top-right-radius: 8px;");
    
    QHBoxLayout *handleLayout = new QHBoxLayout(handle);
    handleLayout->setContentsMargins(10, 0, 10, 0);
    
    QLabel *title = new QLabel("Keyboard Handle", handle);
    title->setStyleSheet("color: #aaa; font-size: 11px; font-weight: bold;");
    
    QPushButton *closeBtn = new QPushButton("✕", handle);
    // closeBtn->setFixedSize(24, 24);
    closeBtn->setStyleSheet("QPushButton { background: transparent; color: white; border: none; font-size: 16px; }"
                            "QPushButton:hover { color: #f44; }");
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);

    handleLayout->addWidget(title);
    handleLayout->addStretch();
    handleLayout->addWidget(closeBtn);
    mainLayout->addWidget(handle);

    // Grid for Keys (Tight spacing)
    QWidget *keysWidget = new QWidget(this);
    QVBoxLayout *keysLayout = new QVBoxLayout(keysWidget);
    keysLayout->setSpacing(4);
    keysLayout->setContentsMargins(6, 6, 6, 6);

    auto addRow = [&](const std::vector<std::tuple<int, QString, QString, float>>& keys) {
        QHBoxLayout *row = new QHBoxLayout();
        row->setSpacing(4);
        for (const auto& k : keys) {
            row->addWidget(createKey(std::get<0>(k), std::get<1>(k), std::get<2>(k), std::get<3>(k)), std::get<3>(k) * 10);
        }
        keysLayout->addLayout(row);
    };

    // Row 1: Numbers
    addRow({{KEY_GRAVE, "`", "~", 1.0}, {KEY_1, "1", "!", 1.0}, {KEY_2, "2", "@", 1.0}, {KEY_3, "3", "#", 1.0}, {KEY_4, "4", "$", 1.0},
            {KEY_5, "5", "%", 1.0}, {KEY_6, "6", "^", 1.0}, {KEY_7, "7", "&", 1.0}, {KEY_8, "8", "*", 1.0}, {KEY_9, "9", "(", 1.0},
            {KEY_0, "0", ")", 1.0}, {KEY_MINUS, "-", "_", 1.0}, {KEY_EQUAL, "=", "+", 1.0}, {KEY_BACKSPACE, "Backspace", "", 2.0}});

    // Row 2: QWERTY
    addRow({{KEY_TAB, "Tab", "", 1.5}, {KEY_Q, "q", "Q", 1.0}, {KEY_W, "w", "W", 1.0}, {KEY_E, "e", "E", 1.0}, {KEY_R, "r", "R", 1.0},
            {KEY_T, "t", "T", 1.0}, {KEY_Y, "y", "Y", 1.0}, {KEY_U, "u", "U", 1.0}, {KEY_I, "i", "I", 1.0}, {KEY_O, "o", "O", 1.0},
            {KEY_P, "p", "P", 1.0}, {KEY_LEFTBRACE, "[", "{", 1.0}, {KEY_RIGHTBRACE, "]", "}", 1.0}, {KEY_BACKSLASH, "\\", "|", 1.5}});

    // Row 3: ASDF
    addRow({{KEY_CAPSLOCK, "CapsLock", "", 1.8}, {KEY_A, "a", "A", 1.0}, {KEY_S, "s", "S", 1.0}, {KEY_D, "d", "D", 1.0}, {KEY_F, "f", "F", 1.0},
            {KEY_G, "g", "G", 1.0}, {KEY_H, "h", "H", 1.0}, {KEY_J, "j", "J", 1.0}, {KEY_K, "k", "K", 1.0}, {KEY_L, "l", "L", 1.0},
            {KEY_SEMICOLON, ";", ":", 1.0}, {KEY_APOSTROPHE, "'", "\"", 1.0}, {KEY_ENTER, "Enter", "", 2.2}});

    // Row 4: ZXCV
    addRow({{KEY_LEFTSHIFT, "Shift", "", 2.3}, {KEY_Z, "z", "Z", 1.0}, {KEY_X, "x", "X", 1.0}, {KEY_C, "c", "C", 1.0}, {KEY_V, "v", "V", 1.0},
            {KEY_B, "b", "B", 1.0}, {KEY_N, "n", "N", 1.0}, {KEY_M, "m", "M", 1.0}, {KEY_COMMA, ",", "<", 1.0}, {KEY_DOT, ".", ">", 1.0},
            {KEY_SLASH, "/", "?", 1.0}, {KEY_RIGHTSHIFT, "Shift", "", 1.7}, {KEY_UP, "↑", "", 1.0}});

    // Row 5: Bottom
    addRow({{KEY_LEFTCTRL, "Ctrl", "", 1.2}, {KEY_LEFTMETA, "Super", "", 1.2}, {KEY_LEFTALT, "Alt", "", 1.2}, {KEY_SPACE, "Space", "", 6.0},
            {KEY_RIGHTALT, "Alt", "", 1.2}, {KEY_RIGHTMETA, "Super", "", 1.2}, {KEY_RIGHTCTRL, "Ctrl", "", 1.2},
            {KEY_LEFT, "←", "", 1.0}, {KEY_DOWN, "↓", "", 1.0}, {KEY_RIGHT, "→", "", 1.0}});

    mainLayout->addWidget(keysWidget);
}

QPushButton *VirtualKeyboard::createKey(int keycode, const QString &label, const QString &shiftLabel, float stretch) {
    QPushButton *btn = new QPushButton(label);
    btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    btn->setMinimumHeight(40);

    m_buttons[keycode] = btn;
    m_keyMap[btn] = {label, shiftLabel.isEmpty() ? label.toUpper() : shiftLabel};

    connect(btn, &QPushButton::pressed, [this, keycode, btn]() {
        if (keycode == KEY_LEFTSHIFT || keycode == KEY_RIGHTSHIFT) {
            m_shiftActive = !m_shiftActive;
            btn->setProperty("active", m_shiftActive);
            btn->style()->unpolish(btn); btn->style()->polish(btn);
            updateKeyCaps();
            sendKey(keycode, m_shiftActive);
        } else if (keycode == KEY_CAPSLOCK) {
            m_capsActive = !m_capsActive;
            btn->setProperty("active", m_capsActive);
            btn->style()->unpolish(btn); btn->style()->polish(btn);
            updateKeyCaps();
            sendKey(keycode, m_capsActive);
        } else {
            sendKey(keycode, true);
        }
    });

    connect(btn, &QPushButton::released, [this, keycode]() {
        if (keycode != KEY_LEFTSHIFT && keycode != KEY_RIGHTSHIFT && keycode != KEY_CAPSLOCK)
            sendKey(keycode, false);
    });

    return btn;
}

void VirtualKeyboard::updateKeyCaps() {
    bool uppercase = m_shiftActive ^ m_capsActive;
    for (auto it = m_buttons.begin(); it != m_buttons.end(); ++it) {
        QPushButton* btn = it.value();
        int code = it.key();
        
        // Don't change labels for special functional keys
        if (code == KEY_BACKSPACE || code == KEY_TAB || code == KEY_ENTER || 
            code == KEY_LEFTSHIFT || code == KEY_RIGHTSHIFT || code == KEY_CAPSLOCK ||
            code >= KEY_LEFTCTRL) continue;

        if (m_shiftActive && !m_keyMap[btn].shift.isEmpty()) {
            btn->setText(m_keyMap[btn].shift);
        } else if (uppercase) {
            btn->setText(m_keyMap[btn].normal.toUpper());
        } else {
            btn->setText(m_keyMap[btn].normal);
        }
    }
}

void VirtualKeyboard::sendKey(int keycode, bool pressed) {
    if (m_uinputFd < 0) return;

    struct input_event ev;
    
    // 1. Send the Key Event
    memset(&ev, 0, sizeof(ev));
    ev.type = EV_KEY;
    ev.code = keycode;
    ev.value = pressed ? 1 : 0;
    if (write(m_uinputFd, &ev, sizeof(ev)) < 0) {
        qDebug() << "Write error:" << strerror(errno);
    }

    // 2. Send the Sync Report (Tells the kernel "I'm done with this update")
    memset(&ev, 0, sizeof(ev));
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    write(m_uinputFd, &ev, sizeof(ev));
}

void VirtualKeyboard::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint();
        m_isResizing = (event->pos().y() < 10);
    }
}

void VirtualKeyboard::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        if (auto lsw = LayerShellQt::Window::get(windowHandle())) {
            QPoint currentPos = event->globalPosition().toPoint();
            QPoint delta = currentPos - m_dragPosition;
            m_dragPosition = currentPos;
            
            if (m_isResizing) {
                int newHeight = height() - delta.y();
                if (newHeight > 100 && newHeight < 600) {
                    setFixedHeight(newHeight);
                }
                return;
            }

            int newBottomMargin = lsw->margins().bottom() - delta.y();
            // Clamp margin so it doesn't go off screen
            newBottomMargin = qMax(0, newBottomMargin);
            
            lsw->setMargins(QMargins(0, 0, 0, newBottomMargin));
        }
        if (m_display) wl_display_flush(m_display);
    }
}
