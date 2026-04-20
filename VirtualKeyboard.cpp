#include "VirtualKeyboard.h"
#include <LayerShellQt/Window>
#include <QDebug>
#include <QGuiApplication>
#include <QStyle>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCursor>
#include <QCoreApplication>
#include <QDir>
#include <linux/input-event-codes.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <vector>
#include <tuple>
#include <QTimer>
#include <fcntl.h>

#include <QStandardPaths>

// QString getSoundPath(const QString &fileName) {
//     // 1. Check for a user-overridden sound in ~/.local/share/cachyboard/sounds/
//     QString userPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/sounds/" + fileName;
//     if (QFile::exists(userPath)) return userPath;

//     // 2. Check for system-installed sounds in /usr/share/cachyboard/sounds/
//     // QStandardPaths::GenericDataLocation usually returns /usr/share and ~/.local/share
//     QStringList systemPaths = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
//     for (const QString &path : systemPaths) {
//         QString fullPath = path + "/cachyboard/sounds/" + fileName;
//         if (QFile::exists(fullPath)) return fullPath;
//     }

//     // 3. Fallback to embedded resource
//     return "qrc:/sounds/click.wav";
// }

VirtualKeyboard::VirtualKeyboard(QWidget *parent) : QWidget(parent) {
	connect(this, &QWidget::destroyed, qApp, &QCoreApplication::quit);
	m_syncTimer = new QTimer(this);
	connect(m_syncTimer, &QTimer::timeout, this, &VirtualKeyboard::syncModifiers);
	m_syncTimer->start(400); // Check every 200ms

	// Basic setup
	setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::Tool);
	setAttribute(Qt::WA_TranslucentBackground);
	this->setMinimumSize(450, 200);
	setMouseTracking(true);
	initUinput();

	setSoundEffect("click");

	// m_clickSound = new QSoundEffect(this);
	// // Make sure to place a click.wav in your build directory or use a resource path
	// QString soundPath = QStringLiteral(":/sounds/click.wav");

	// // Check if it exists for debugging
	// if (!QFile::exists(soundPath)) {
	// 	qWarning() << "Sound file NOT found at:" << soundPath;
	// }

	// m_clickSound->setSource(QUrl::fromLocalFile(soundPath));
	// m_clickSound->setVolume(0.5f);

	setupUI();
	// resize(800, 300);

	setStyleSheet(
		"QWidget { background-color: rgba(35, 35, 35, 240); border-radius: 0px; }"
		"QPushButton { background-color: #444; color: white; border: 1px solid #555; "
		"border-radius: 4px; padding: 5px; font-size: 14pt; font-weight: bold; min-width: 30px; }"
		"QPushButton:pressed { background-color: #0078d7; }"
		"QPushButton[active='true'] { background-color: #005a9e; border: 1px solid #00a2ff; }");
}


VirtualKeyboard::~VirtualKeyboard() {
	if (m_uinputFd == -1) return; // Already cleaned up
	qDebug() << "Running destructor";
	qDebug() << "m_uinputFd is" << m_uinputFd;

	if (m_uinputFd >= 0) {
		// Force release all keys before destroying the device
		releaseAllKeys();
		// Add usleep(50000); (50ms) before you close the file descriptor 
		// to ensure the "all release" signals are flushed.
		usleep(50000);

		ioctl(m_uinputFd, UI_DEV_DESTROY, 0);
		
		// Use :: to specify the global Linux close() function
		::close(m_uinputFd); 
		
		m_uinputFd = -1;
		qDebug() << "uinput device cleaned up successfully.";
	}
}


void VirtualKeyboard::releaseAllKeys() {
	qDebug() << "releaseAllKeys()";
	qDebug() << "m_uinputFd is" << m_uinputFd;

	if (m_uinputFd < 0) return;
	
	// Scan through standard keycodes and force release
	// KEY_MAX is defined in linux/input.h
	for (int i = 0; i <= KEY_MAX; i++) {
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
		qCritical() << "CRITICAL: Could not open /dev/uinput:" << strerror(errno);
		return;
	}

	// Enable Key Events
	ioctl(m_uinputFd, UI_SET_EVBIT, EV_KEY);
	
	// Enable ALL keys (0 to KEY_MAX covers all standard Linux keycodes)
	for (int i = 0; i < KEY_MAX; i++) {
		ioctl(m_uinputFd, UI_SET_KEYBIT, i);
	}

	// Setup Device Info
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

			// Set an exclusive zone of 0 so it doesn't push other windows up,
			// or a positive value equal to height if you want it to push apps up.
			// Remove AnchorLeft/Right so width isn't forced to screen size
			// Anchor to Bottom AND Left for 2D movement
			// Explicitly use the Anchors constructor
			LayerShellQt::Window::Anchors anchors(LayerShellQt::Window::AnchorTop | LayerShellQt::Window::AnchorLeft);
			lsw->setAnchors(anchors);
			lsw->setExclusiveZone(0);
			
			// Force the height so it doesn't fill the screen
			resize(QGuiApplication::primaryScreen()->size().width() * 0.8, 300);

			// Position it at the bottom of the screen initially using margins
			int screenHeight = QGuiApplication::primaryScreen()->size().height();
			int screenWidth = QGuiApplication::primaryScreen()->size().width();
			int keyboardWidth = screenWidth * 0.8; 
			int leftMargin = (screenWidth - keyboardWidth) / 2; // Center math
			lsw->setMargins(QMargins(leftMargin, screenHeight - 350, leftMargin, 0));
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
	// Removed rounded corners to make it look like a handle, not a window
	handle->setStyleSheet("background-color: #333; border-radius: 0px;");
	
	QHBoxLayout *handleLayout = new QHBoxLayout(handle);
	handleLayout->setContentsMargins(10, 0, 10, 0);
	
	QLabel *title = new QLabel(APP_NAME, handle);
	title->setStyleSheet("color: #aaa; font-size: 11px; font-weight: bold;");
	
	QPushButton *closeBtn = new QPushButton("✕", handle);
	// closeBtn->setFixedSize(24, 24);
	closeBtn->setStyleSheet("QPushButton { background: transparent; color: white; border: none; font-size: 16px;}"
							"QPushButton:hover { color: #f44; }");
	connect(closeBtn, &QPushButton::clicked, qApp, &QCoreApplication::quit);

	m_resizeHandle = new QPushButton("◢", this);
	m_resizeHandle->setFixedSize(20, 20);
	m_resizeHandle->setCursor(Qt::SizeFDiagCursor);
	// Let mouse events pass through the button to the VirtualKeyboard 
	// so our custom drag/resize logic in mousePressEvent works correctly.
	m_resizeHandle->setAttribute(Qt::WA_TransparentForMouseEvents);
	m_resizeHandle->setStyleSheet("background: transparent; color: #666; border: none;");

	handleLayout->addWidget(title);
	handleLayout->addStretch();
	handleLayout->addWidget(closeBtn);
	handleLayout->addWidget(m_resizeHandle);
	mainLayout->addWidget(handle);

	// Grid for Keys (Tight spacing)
	QWidget *keysWidget = new QWidget(this);
	QVBoxLayout *keysLayout = new QVBoxLayout(keysWidget);
	// keysLayout->setSpacing(4);
	// keysLayout->setContentsMargins(6, 6, 6, 6);

	auto addRow = [&](const std::vector<std::tuple<int, QString, QString, float>>& keys) {
		QHBoxLayout *row = new QHBoxLayout();
		// row->setSpacing(4);
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
	addRow({{KEY_CAPSLOCK, "CapsLock", "", 2}, {KEY_A, "a", "A", 1.0}, {KEY_S, "s", "S", 1.0}, {KEY_D, "d", "D", 1.0}, {KEY_F, "f", "F", 1.0},
			{KEY_G, "g", "G", 1.0}, {KEY_H, "h", "H", 1.0}, {KEY_J, "j", "J", 1.0}, {KEY_K, "k", "K", 1.0}, {KEY_L, "l", "L", 1.0},
			{KEY_SEMICOLON, ";", ":", 1.0}, {KEY_APOSTROPHE, "'", "\"", 1.0}, {KEY_ENTER, "Enter", "", 2}});

	// Row 4: ZXCV
	addRow({{KEY_LEFTSHIFT, "Shift", "", 2.5}, {KEY_Z, "z", "Z", 1.0}, {KEY_X, "x", "X", 1.0}, {KEY_C, "c", "C", 1.0}, {KEY_V, "v", "V", 1.0},
			{KEY_B, "b", "B", 1.0}, {KEY_N, "n", "N", 1.0}, {KEY_M, "m", "M", 1.0}, {KEY_COMMA, ",", "<", 1.0}, {KEY_DOT, ".", ">", 1.0},
			{KEY_SLASH, "/", "?", 1.0}, {KEY_RIGHTSHIFT, "Shift", "", 1.5}, {KEY_UP, "↑", "", 1.0}});

	// Row 5: Bottom
	addRow({{KEY_LEFTCTRL, "Ctrl", "", 1.0}, {KEY_LEFTMETA, "Super", "", 1.0}, {KEY_LEFTALT, "Alt", "", 1.0}, {KEY_SPACE, "Space", "", 6.3},
			{KEY_RIGHTALT, "Alt", "", 1.0}, {KEY_RIGHTMETA, "Super", "", 1.0}, {KEY_RIGHTCTRL, "Ctrl", "", 1.0},
			{KEY_LEFT, "←", "", 1.0}, {KEY_DOWN, "↓", "", 1.0}, {KEY_RIGHT, "→", "", 1.0}});

	mainLayout->addWidget(keysWidget);
}


QPushButton *VirtualKeyboard::createKey(int keycode, const QString &label, const QString &shiftLabel, float stretch) {
	QPushButton *btn = new QPushButton(label);

	// Minimum ensures the button won't shrink smaller than setMinimumWidth
    btn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    btn->setMinimumHeight(40);
	btn->setMinimumWidth(40); 
    
	m_buttons[keycode] = btn;
	m_keyMap[btn] = {label, shiftLabel.isEmpty() ? label.toUpper() : shiftLabel};

	connect(btn, &QPushButton::pressed, [this, keycode, btn]() {
		if (m_clickSound->isLoaded()) m_clickSound->play();
		// syncModifiers();

		if (keycode == KEY_LEFTSHIFT || keycode == KEY_RIGHTSHIFT) {
			// Shift (The Modifier): Shift is not a switch; it is a modifier. 
			// It is only active while the key is physically held down. 
			// If you send Press + Release instantly, the Shift state is gone 
			// before you even click the next letter. To make it work, you must send 
			// "Press" (true) and not send "Release" until the user clicks Shift again to turn it off.
			m_shiftActive = !m_shiftActive;

			// Hold the key down in the OS if active, release if inactive
			sendKey(keycode, m_shiftActive); 
			
			btn->setProperty("active", m_shiftActive);
			btn->style()->unpolish(btn);
			btn->style()->polish(btn);
			updateKeyCaps();
                        
		} else if (keycode == KEY_CAPSLOCK) {
			// Caps Lock (The Toggle): The kernel treats Caps Lock as a "switch." 
			// When it receives a Press + Release sequence, it toggles the "Caps" state on. 
			// It stays on until it receives another Press + Release. 
			// If you only send "Press" (true) without "Release" (false), the OS thinks a physical 
			// finger is permanently holding the button down, which is why your hardware keyboard gets "stuck".

			// 1. Toggle our internal UI state
			m_capsActive = !m_capsActive;

			// 2. ALWAYS send a full Press + Release to the OS to trigger the toggle
			sendKey(keycode, true);
			sendKey(keycode, false);

			// 3. Update the button's visual "active" blue highlight
			btn->setProperty("active", m_capsActive);
			btn->style()->unpolish(btn); 
			btn->style()->polish(btn);
			updateKeyCaps();
                        
		} else {
			// Regular keys release on the released() signal below
			sendKey(keycode, true);
		}
	});

	connect(btn, &QPushButton::released, [this, keycode]() {
		if (keycode != KEY_LEFTSHIFT && keycode != KEY_RIGHTSHIFT && keycode != KEY_CAPSLOCK)
			sendKey(keycode, false);
	});

	return btn;
}


// Replacement for the missing macro
#define MY_BITS_TO_LONGS(nr) (((nr) + (8 * sizeof(long)) - 1) / (8 * sizeof(long)))

void VirtualKeyboard::syncModifiers() {
    int fd = ::open("/dev/input/hw_kbd", O_RDONLY);
    if (fd == -1) return;

    // Check CAPS LOCK (LED state)
    unsigned long led_state[MY_BITS_TO_LONGS(LED_CNT)] = {0};
    if (ioctl(fd, EVIOCGLED(sizeof(led_state)), led_state) >= 0) {
        bool capsOn = (led_state[LED_CAPSL / (8 * sizeof(long))] & (1UL << (LED_CAPSL % (8 * sizeof(long)))));

        if (m_capsActive != capsOn) {
            m_capsActive = capsOn;
			updateKeyCaps();
        }
    }

    // Check SHIFT (Key state)
    // KEY_MAX is usually 0x2ff. We check if either Left or Right shift is depressed.
    unsigned long key_state[MY_BITS_TO_LONGS(KEY_MAX)] = {0};
    if (ioctl(fd, EVIOCGKEY(sizeof(key_state)), key_state) >= 0) {
        bool leftShift = (key_state[KEY_LEFTSHIFT / (8 * sizeof(long))] & (1UL << (KEY_LEFTSHIFT % (8 * sizeof(long)))));
        bool rightShift = (key_state[KEY_RIGHTSHIFT / (8 * sizeof(long))] & (1UL << (KEY_RIGHTSHIFT % (8 * sizeof(long)))));
        bool anyShift = leftShift || rightShift;

        if (m_shiftActive != anyShift) {
            m_shiftActive = anyShift;
            updateKeyCaps();
        }
    }
    ::close(fd);
}

// void VirtualKeyboard::syncModifiers() {
// 	// Qt::KeyboardModifiers mods = QGuiApplication::queryKeyboardModifiers();
// 	// bool hardwareShift = mods.testFlag(Qt::ShiftModifier);

// 	// CapsLock state is not part of Qt::KeyboardModifiers.
// 	// In a cross-platform way, you can check if the CapsLock key is currently 
// 	// toggled using the QKeyEvent logic, but for a simple sync:
// 	// bool hardwareCaps = QGuiApplication::keyboardModifiers().testFlag(Qt::GroupSwitchModifier); 
// 	// Note: On many Linux systems, CapsLock is not easily queried via queryKeyboardModifiers.
	
// 	// Recommended Fix: Only sync Shift automatically, and let CapsLock be handled 
// 	// by the toggle state of your virtual button, or use:
// 	// bool hardwareCaps = (QGuiApplication::queryKeyboardModifiers() & Qt::KeypadModifier); // Platform dependent

// 	auto mods = QGuiApplication::queryKeyboardModifiers();
//     // int hardwareCaps = mods.testFlag(Qt::GroupSwitchModifier);
//     int hardwareShift = mods.testFlag(Qt::ShiftModifier);
// 	// Instead of Qt::CapsLockModifier, try:
// 	bool hardwareCaps = QGuiApplication::queryKeyboardModifiers().testFlag(Qt::GroupSwitchModifier); 
// 	// Note: In some Qt versions, Caps Lock is handled via the queryKeyboardModifiers() 
// 	// return value where you check the specific bit.

// 	qDebug() << "HW Shift:" << hardwareShift;
// 	qDebug() << "HW Caps:" << hardwareCaps;

// 	// if (hardwareShift != m_shiftActive || hardwareCaps != m_capsActive) {
// 	// 	m_shiftActive = hardwareShift;
// 	// 	m_capsActive = hardwareCaps;
		
// 	// 	if (m_buttons.contains(KEY_LEFTSHIFT)) m_buttons[KEY_LEFTSHIFT]->setProperty("active", m_shiftActive);
// 	// 	if (m_buttons.contains(KEY_CAPSLOCK)) m_buttons[KEY_CAPSLOCK]->setProperty("active", m_capsActive);
		
// 	// 	for (auto btn : m_buttons) {
// 	// 		btn->style()->unpolish(btn); 
// 	// 		btn->style()->polish(btn);
// 	// 	}
// 	// 	updateKeyCaps();
// 	// }
// }


void VirtualKeyboard::updateKeyCaps() {
  	bool uppercase = m_shiftActive ^ m_capsActive;

	for (auto it = m_buttons.begin(); it != m_buttons.end(); ++it) {
		QPushButton* btn = it.value();
		int code = it.key();

		bool shouldHighlight = false;

        if (code == KEY_CAPSLOCK) {
            shouldHighlight = m_capsActive;
        } else if (code == KEY_LEFTSHIFT || code == KEY_RIGHTSHIFT) {
            shouldHighlight = m_shiftActive;
        }

        // Only update if the property actually changed to prevent flicker
        if (btn->property("active").toBool() != shouldHighlight) {
            btn->setProperty("active", shouldHighlight);
            btn->style()->unpolish(btn);
            btn->style()->polish(btn);
        }

		// Don't change labels for special functional keys
		// Skip only specific functional keys
		if (code < 1 || code == KEY_BACKSPACE || code == KEY_TAB || code == KEY_ENTER || 
			code == KEY_LEFTSHIFT || code == KEY_RIGHTSHIFT || code == KEY_CAPSLOCK ||
			code == KEY_LEFTCTRL || code == KEY_RIGHTCTRL || code == KEY_LEFTALT || 
			code == KEY_RIGHTALT || code == KEY_LEFTMETA || code == KEY_RIGHTMETA || code == KEY_SPACE
		) continue;
		
		if (m_shiftActive && !m_keyMap[btn].shift.isEmpty()) {
			QString text = m_keyMap[btn].shift;
			if (text == "&") text = "&&"; // Escape the ampersand
			btn->setText(text);

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


void VirtualKeyboard::setSoundEffect(const QString &soundName) {
    if (soundName == "None") {
        m_clickSound->setSource(QUrl()); // Clear source to turn off sound
        return;
    }

    // Construct the resource path
    QString resPath = QString("qrc:/sounds/%1.wav").arg(soundName);
    m_clickSound->setSource(QUrl(resPath));
	m_clickSound->setVolume(0.5f);

	// Check if it exists for debugging
	if (!QFile::exists(resPath)) {
		qWarning() << "Sound file NOT found at:" << resPath;
	}
}


void VirtualKeyboard::mousePressEvent(QMouseEvent *event) {
	if (event->button() == Qt::LeftButton) {
		// Record where the drag started
		m_dragPosition = event->globalPosition().toPoint();
		
		// Check if we clicked the Resize Handle
		QRect resizeRect = m_resizeHandle->rect().translated(m_resizeHandle->mapTo(this, QPoint(0, 0)));
		
		if (resizeRect.contains(event->pos())) {
			m_currentEdge = Right;
		} else {
			// We'll treat the entire top handle as the "Move" button
			m_currentEdge = Move;
		}
	}
}


void VirtualKeyboard::mouseMoveEvent(QMouseEvent *event) {
	if (event->buttons() & Qt::LeftButton) {
		if (auto lsw = LayerShellQt::Window::get(windowHandle())) {
			QPoint currentGlobalPos = event->globalPosition().toPoint();
			QPoint delta = currentGlobalPos - m_dragPosition;
			// Note: We DO NOT update m_dragPosition here if we want a single delta 
			// from the start of the click to the release.

			if (m_currentEdge == Right) {
				// Keep resize in real-time as it's usually smoother than moving
				int nw = qMax(450, width() + delta.x());
				int nh = qMax(250, height() + delta.y());
				this->setFixedSize(nw, nh);

				// Updating this when moving will add lag
				m_dragPosition = currentGlobalPos; // Update for next move increment

			}else if (m_currentEdge == Move) {
				QPoint currentGlobalPos = event->globalPosition().toPoint();
                QPoint delta = currentGlobalPos - m_dragPosition;
                
                QMargins m = lsw->margins();
                lsw->setMargins(QMargins(m.left() + delta.x(), m.top() + delta.y(), m.right(), m.bottom()));
                
				// Force the compositor to recognize the change
				// This triggers a buffer commit in the Qt Wayland backend
				this->update(); 

				// If this->update() doesn't snap it into place immediately, try 
				// windowHandle()->requestUpdate();
				
				// Optional: Re-applying the size often forces a layout sync in KWin/Wlroots
				// this->resize(this->size());
			}
		}
	}
}


void VirtualKeyboard::mouseReleaseEvent(QMouseEvent *event) {
    //if (event->button() == Qt::LeftButton && m_currentEdge == Move) {
        // if (auto lsw = LayerShellQt::Window::get(windowHandle())) {
        //     QPoint delta = event->globalPosition().toPoint() - m_dragPosition;
        //     QMargins m = lsw->margins();

        //     int newLeft = qMax(0, m.left() + delta.x());
        //     int newTop = qMax(0, m.top() + delta.y());

        //     // 1. Update margins
        //     lsw->setMargins(QMargins(newLeft, newTop, m.right(), m.bottom()));

        //     // 2. FORCE the compositor to recognize the change
        //     // This triggers a buffer commit in the Qt Wayland backend
        //     this->update(); 
            
        //     // 3. Optional: Re-applying the size often forces a layout sync in KWin/Wlroots
        //     this->resize(this->size());
        // }
    //}

    m_currentEdge = None;
    setCursor(Qt::ArrowCursor);
}


void VirtualKeyboard::resizeEvent(QResizeEvent *event) {
	QWidget::resizeEvent(event);

	// Simple heuristic: font size is 5% of window height, clamped between 8pt and 16pt
    // int newFontSize = qBound(8, event->size().height() / 20, 16);
    
    // // Apply to all buttons
    // for (auto btn : m_buttons) {
    //     QFont f = btn->font();
    //     f.setPointSize(newFontSize);
    //     btn->setFont(f);
    // }
}