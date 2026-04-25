#ifndef VIRTUALKEYBOARD_H
#define VIRTUALKEYBOARD_H

#include <QGridLayout>
#include <QMouseEvent>
#include <QPoint>
#include <QPushButton>
#include <QWidget>
#include <linux/uinput.h>
#include <QtMultimedia/QSoundEffect>
#include <QMap>
#include <QStackedWidget>
#include <QComboBox>
#include <QSettings>
#include <fcntl.h>

class VirtualKeyboard : public QWidget {
  	Q_OBJECT

public:
	VirtualKeyboard(QWidget *parent = nullptr);
	~VirtualKeyboard();

protected:
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void showEvent(QShowEvent *event) override;

private:
	void initUinput();
	void setupUI();
	void setupSettingsUI();
	void toggleSettings();
	void loadSettings();
	void saveSettings();
	void sendKey(int keycode, bool pressed);
	void sendCombo(int modCommand, int key);
	void releaseAllKeys();
	void updateKeyCaps();
	void syncModifiers();
	void updateButtonHighlights();
	void setSoundEffect(const QString &soundName);
	QPushButton *createKey(int keycode, const QString &label, const QString &shiftLabel = QString(), float stretch = 1.0);

	int m_uinputFd = -1;
	QWidget* m_dragHandle;
	QPushButton *m_resizeHandle;
	QPushButton *m_settingsBtn;
	QStackedWidget *m_stackedWidget;
	QWidget *m_settingsWidget;
	QComboBox *m_soundCombo;
	QComboBox *m_styleCombo;
	QSoundEffect* m_clickSound;
	QTimer *m_syncTimer;

	// Platform state
	QPoint m_dragPosition;
	enum ResizeEdge { None, Move, Top, Left, Right, Bottom };
	ResizeEdge m_currentEdge = None;

	struct KeyInfo {
		QString normal;
		QString shift;
	};
	QMap<int, QPushButton*> m_buttons;
	QMap<QPushButton*, KeyInfo> m_keyMap;

	// Modifiers state
	bool m_shiftActive = false;
	bool m_altActive = false;
	bool m_ctrlActive = false;
	bool m_metaActive = false;
	bool m_capsActive = false;
};

#endif
