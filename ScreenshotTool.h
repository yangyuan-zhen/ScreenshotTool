#ifndef SCREENSHOTTOOL_H
#define SCREENSHOTTOOL_H

#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QDateTime>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>

class ScreenshotTool : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenshotTool(QWidget *parent = nullptr);
    ~ScreenshotTool();

    void start();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onWidthChanged(const QString &text);
    void onHeightChanged(const QString &text);
    void onCustomSizeConfirmed();
    void onCustomSizeCancelled();

private:
    void takeScreenshot();
    void saveScreenshot();
    void copyToClipboard();
    void resetSelection();
    QString generateFileName() const;
    void showSizeInputDialog();
    void updateSelectedAreaFromInputs();
    void hideCustomSizeControls();

private:
    bool m_isSelecting;
    QPoint m_startPos;
    QPoint m_endPos;
    QPixmap m_fullScreenPixmap;
    QPixmap m_selectedPixmap;
    QRect m_selectedArea;
    
    // 自定义尺寸相关控件
    QWidget *m_customSizeWidget;
    QLineEdit *m_widthEdit;
    QLineEdit *m_heightEdit;
    QLabel *m_sizeLabel;
    QPushButton *m_confirmButton;
    QPushButton *m_cancelButton;
    bool m_isCustomSizing;
};

#endif // SCREENSHOTTOOL_H 