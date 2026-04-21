#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QStringList>
#include <QMap>

namespace Config {
    // List of available sounds (filenames in qrc:/sounds/)
    inline const QStringList Sounds = {"None", "click"};

    // Dictionary of available styles
    inline const QMap<QString, QString> Styles = {
        {"Dark", 
            "QWidget { background-color: rgba(35, 35, 35, 240); border-radius: 0px; }"
            "QPushButton { "
			"  background-color: #444; color: white; border: 1px solid #555; "
            "  border-radius: 4px; padding: 5px; font-size: 14pt; font-weight: bold; min-width: 30px; "
			"}"
            "QPushButton:pressed { background-color: #0078d7; }"
            "QPushButton[active='true'] { background-color: #005a9e; border: 1px solid #00a2ff; }"
            "QLabel { color: #888;}"
            "QComboBox { background-color: #222; color: white; border: 1px solid #444; padding: 5px; border-radius: 3px; }"
			"QComboBox QAbstractItemView { background-color: #444; color: white; selection-background-color: #0078d7; }"
        },

		{"Dark 3D", 
			"QWidget { background-color: rgba(30, 30, 30, 240); border-radius: 0px; }"
			"QPushButton { "
			"  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #555, stop:1 #333); "
			"  color: #eee; "
			"  border-top: 1px solid #666; "
			"  border-left: 1px solid #555; "
			"  border-right: 2px solid #111; "
			"  border-bottom: 5px solid #000; "
			"  border-radius: 5px; "
			"  padding: 5px; font-size: 14pt; font-weight: bold; min-width: 30px; "
			"}"
			"QPushButton:pressed { "
			"  background-color: #222; "
			"  border-bottom: 1px solid #000; "
			"  margin-top: 4px; "
			"}"
			"QPushButton[active='true'] { "
			"  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0078d7, stop:1 #004a80); "
			"  color: white; "
			"  border: 1px solid #00bcff; "
			"  border-bottom: 4px solid #002a4d; "
			"}"
			"QLabel { color: #888;}"
			"QComboBox { background-color: #222; color: white; border: 1px solid #444; padding: 5px; border-radius: 3px; }"
			"QComboBox QAbstractItemView { background-color: #444; color: white; selection-background-color: #0078d7; }"
		},

        {"Midnight Blue", 
            "QWidget { background-color: rgba(10, 25, 45, 240); }"
            "QPushButton { background-color: #1a3a5f; color: #8ab4f8; border: 1px solid #2a4d7d; }"
            "QPushButton:pressed { background-color: #3a6ea5; }"
            "QPushButton[active='true'] { background-color: #4d90fe; color: white; }"
            "QLabel { color: #8ab4f8; }"
			"QComboBox { background-color: #1a3a5f; color: #8ab4f8; border: 1px solid #2a4d7d; padding: 5px; border-radius: 3px; }"
			"QComboBox QAbstractItemView { background-color: #1a3a5f; color: #8ab4f8; selection-background-color: #440000; border: 1px solid #2a4d7d; }"
        },

		{"Twilight", 
			"QWidget { background-color: rgba(30, 30, 30, 240); border: 0px solid #333; }"
			
			// Default Key State: Dark gray with red text and a "beveled" 3D bottom
			"QPushButton { "
			"  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #444, stop:1 #222); "
			"  color: #ff4444; "
			"  border-top: 1px solid #555; "
			"  border-left: 1px solid #555; "
			"  border-right: 2px solid #111; "
			"  border-bottom: 4px solid #000; " // The 3D "thickness"
			"  border-radius: 6px; "
			"  padding: 5px; font-size: 14pt; font-weight: bold; min-width: 35px; "
			"}"
			
			// Pressed State: Moves "down" (reduce bottom border)
			"QPushButton:pressed { "
			"  background-color: #111; "
			"  border-bottom: 1px solid #000; "
			"  margin-top: 3px; "
			"}"
			
			// Active/Hardware Sync State: The "Glow" effect
			"QPushButton[active='true'] { "
			"  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #600, stop:1 #300); "
			"  color: #ff9999; "
			"  border: 1px solid #ff0000; "
			"  border-bottom: 4px solid #200; "
			"  /* Outer glow simulation */"
			"  outline: none; "
			"}"
			
			"QLabel { color: #ff4444; }"
			"QComboBox { background-color: #222; color: #ff4444; border: 1px solid #ff4444; padding: 5px; border-radius: 3px; }"
			"QComboBox QAbstractItemView { background-color: #111; color: #ff4444; selection-background-color: #440000; border: 1px solid #ff4444; }"
		},

		{"Twilight v2", 
			"QWidget { background-color: rgba(30, 30, 30, 240); border: 0px solid #333; }"
			"QPushButton { "
			"  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3a3a3a, stop:1 #1a1a1a); "
			"  color: #ff4444; " // Saturated Red
			"  border-top: 1px solid #555; "
			"  border-left: 1px solid #444; "
			"  border-right: 2px solid #000; "
			"  border-bottom: 5px solid #000; "
			"  border-radius: 6px; "
			"  padding: 5px; font-size: 14pt; font-weight: bold; min-width: 35px; "
			"}"
			"QPushButton:pressed { "
			"  background-color: #111; "
			"  border-bottom: 1px solid #000; "
			"  margin-top: 4px; "
			"}"
			"QPushButton[active='true'] { "
			"  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ff0000, stop:1 #800000); "
			"  color: white; " // White text makes it look like the LED is "maxed out"
			"  border: 2px solid #ff6666; "
			"  border-bottom: 3px solid #400; "
			"}"

			"QLabel { color: #ff4444; }"
			"QComboBox { background-color: #222; color: #ff4444; border: 1px solid #ff4444; padding: 5px; border-radius: 3px; }"
			"QComboBox QAbstractItemView { background-color: #111; color: #ff4444; selection-background-color: #440000; border: 1px solid #ff4444; }"
		},

		{"Supernova Red", 
			"QWidget { background-color: rgba(10, 10, 10, 250); border: 1px solid #222; }"
			
			// Default state is now the "Glow" look
			"QPushButton { "
			"  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #800, stop:1 #300); "
			"  color: #ff9999; "
			"  border: 1px solid #ff0000; "
			"  border-bottom: 5px solid #200; "
			"  border-radius: 6px; "
			"  padding: 5px; font-size: 14pt; font-weight: bold; min-width: 35px; "
			"}"
			
			// Physical press effect: Sinks into the board and turns bright white-red
			"QPushButton:pressed { "
			"  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ff4444, stop:1 #aa0000); "
			"  color: white; "
			"  border-bottom: 1px solid #000; "
			"  margin-top: 4px; "
			"}"
			
			// Active state (Hardware Sync): Turns into an "Overdrive" mode (White text/Yellow-Red glow)
			"QPushButton[active='true'] { "
			"  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ff0000, stop:1 #600); "
			"  color: #ffffff; "
			"  border: 2px solid #ffffff; "
			"  border-bottom: 4px solid #400; "
			"}"
			
			"QLabel { color: #ff0000; font-size: 11pt; font-weight: bold; }"
			
			"QComboBox { background-color: #200; color: #ff9999; border: 1px solid #ff0000; padding: 4px; }"
			"QComboBox QAbstractItemView { background-color: #200; color: #ff9999; selection-background-color: #600; }"
		}
    };

    inline const QString DefaultStyle = "Dark";
    inline const QString DefaultSound = "click";
}

#endif // CONFIG_H