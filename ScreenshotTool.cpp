#include "ScreenshotTool.h"

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QScreen>
#include <QFileDialog>
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIntValidator>

ScreenshotTool::ScreenshotTool(QWidget *parent)
    : QWidget(parent)
    , m_isSelecting(false)
    , m_isCustomSizing(false)
    , m_customSizeWidget(nullptr)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setCursor(Qt::CrossCursor);
    
    // 创建自定义尺寸输入控件
    m_customSizeWidget = new QWidget(this);
    m_customSizeWidget->setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    m_customSizeWidget->setAttribute(Qt::WA_TranslucentBackground, false);
    m_customSizeWidget->setStyleSheet("background-color: rgba(45, 45, 45, 220); color: white; border-radius: 5px;");
    m_customSizeWidget->setFixedSize(250, 150);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_customSizeWidget);
    
    m_sizeLabel = new QLabel("输入自定义截图尺寸", m_customSizeWidget);
    m_sizeLabel->setAlignment(Qt::AlignCenter);
    m_sizeLabel->setStyleSheet("font-weight: bold; font-size: 14px; padding: 5px;");
    
    QHBoxLayout *inputLayout = new QHBoxLayout();
    
    QLabel *widthLabel = new QLabel("宽度:", m_customSizeWidget);
    m_widthEdit = new QLineEdit(m_customSizeWidget);
    m_widthEdit->setValidator(new QIntValidator(1, 9999, this));
    m_widthEdit->setFixedWidth(60);
    m_widthEdit->setStyleSheet("background-color: white; color: black; padding: 5px;");
    connect(m_widthEdit, &QLineEdit::textChanged, this, &ScreenshotTool::onWidthChanged);
    
    QLabel *heightLabel = new QLabel("高度:", m_customSizeWidget);
    m_heightEdit = new QLineEdit(m_customSizeWidget);
    m_heightEdit->setValidator(new QIntValidator(1, 9999, this));
    m_heightEdit->setFixedWidth(60);
    m_heightEdit->setStyleSheet("background-color: white; color: black; padding: 5px;");
    connect(m_heightEdit, &QLineEdit::textChanged, this, &ScreenshotTool::onHeightChanged);
    
    inputLayout->addWidget(widthLabel);
    inputLayout->addWidget(m_widthEdit);
    inputLayout->addSpacing(10);
    inputLayout->addWidget(heightLabel);
    inputLayout->addWidget(m_heightEdit);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_confirmButton = new QPushButton("确认", m_customSizeWidget);
    m_confirmButton->setCursor(Qt::PointingHandCursor);
    m_confirmButton->setStyleSheet("background-color: #4CAF50; padding: 5px 15px; border: none; border-radius: 3px;");
    connect(m_confirmButton, &QPushButton::clicked, this, &ScreenshotTool::onCustomSizeConfirmed);
    
    m_cancelButton = new QPushButton("取消", m_customSizeWidget);
    m_cancelButton->setCursor(Qt::PointingHandCursor);
    m_cancelButton->setStyleSheet("background-color: #f44336; padding: 5px 15px; border: none; border-radius: 3px;");
    connect(m_cancelButton, &QPushButton::clicked, this, &ScreenshotTool::onCustomSizeCancelled);
    
    buttonLayout->addWidget(m_confirmButton);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addWidget(m_sizeLabel);
    mainLayout->addLayout(inputLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch();
    
    m_customSizeWidget->hide();
}

ScreenshotTool::~ScreenshotTool()
{
}

void ScreenshotTool::start()
{
    // 获取主屏幕
    QScreen *screen = QGuiApplication::primaryScreen();
    
    // 截取全屏
    m_fullScreenPixmap = screen->grabWindow(0);
    
    // 设置窗口大小为全屏
    setGeometry(screen->geometry());
    
    // 重置选区
    resetSelection();
    
    // 显示全屏窗口
    showFullScreen();
}

void ScreenshotTool::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    
    // 绘制全屏截图作为背景
    painter.drawPixmap(0, 0, m_fullScreenPixmap);
    
    // 绘制半透明遮罩
    painter.fillRect(rect(), QColor(0, 0, 0, 100));
    
    if ((m_isSelecting || !m_selectedArea.isEmpty()) && !m_isCustomSizing) {
        // 计算选区矩形
        QRect selectedRect = m_selectedArea;
        if (m_isSelecting) {
            selectedRect = QRect(
                qMin(m_startPos.x(), m_endPos.x()),
                qMin(m_startPos.y(), m_endPos.y()),
                qAbs(m_endPos.x() - m_startPos.x()),
                qAbs(m_endPos.y() - m_startPos.y())
            );
        }
        
        if (!selectedRect.isEmpty()) {
            // 清除选区的遮罩（显示原始图像）
            painter.drawPixmap(selectedRect, m_fullScreenPixmap, selectedRect);
            
            // 绘制选区边框
            QPen pen(Qt::red, 2);
            painter.setPen(pen);
            painter.drawRect(selectedRect);
            
            // 绘制选区大小信息
            QString sizeText = QString("%1 x %2").arg(selectedRect.width()).arg(selectedRect.height());
            QFont font = painter.font();
            font.setBold(true);
            painter.setFont(font);
            
            // 文字背景
            QRect textRect = painter.fontMetrics().boundingRect(sizeText);
            textRect.adjust(-5, -5, 5, 5);
            textRect.moveTopLeft(QPoint(selectedRect.left(), selectedRect.top() - textRect.height()));
            if (textRect.top() < 0) {
                textRect.moveTop(selectedRect.bottom());
            }
            
            painter.fillRect(textRect, QColor(0, 0, 0, 180));
            painter.setPen(Qt::white);
            painter.drawText(textRect, Qt::AlignCenter, sizeText);
        }
    }
    else if (!m_selectedArea.isEmpty() && m_isCustomSizing) {
        // 绘制自定义尺寸选区
        painter.drawPixmap(m_selectedArea, m_fullScreenPixmap, m_selectedArea);
        
        // 绘制选区边框
        QPen pen(Qt::blue, 2); // 使用蓝色区分自定义尺寸模式
        painter.setPen(pen);
        painter.drawRect(m_selectedArea);
        
        // 绘制选区大小信息
        QString sizeText = QString("%1 x %2 (自定义)").arg(m_selectedArea.width()).arg(m_selectedArea.height());
        QFont font = painter.font();
        font.setBold(true);
        painter.setFont(font);
        
        // 文字背景
        QRect textRect = painter.fontMetrics().boundingRect(sizeText);
        textRect.adjust(-5, -5, 5, 5);
        textRect.moveTopLeft(QPoint(m_selectedArea.left(), m_selectedArea.top() - textRect.height()));
        if (textRect.top() < 0) {
            textRect.moveTop(m_selectedArea.bottom());
        }
        
        painter.fillRect(textRect, QColor(0, 0, 0, 180));
        painter.setPen(Qt::white);
        painter.drawText(textRect, Qt::AlignCenter, sizeText);
    }
}

void ScreenshotTool::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !m_isCustomSizing) {
        m_isSelecting = true;
        m_startPos = event->pos();
        m_endPos = m_startPos;
        m_selectedArea = QRect();
        update();
    }
}

void ScreenshotTool::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isSelecting && !m_isCustomSizing) {
        m_endPos = event->pos();
        update();
    }
}

void ScreenshotTool::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_isSelecting && !m_isCustomSizing) {
        m_isSelecting = false;
        m_endPos = event->pos();
        
        // 计算选区
        m_selectedArea = QRect(
            qMin(m_startPos.x(), m_endPos.x()),
            qMin(m_startPos.y(), m_endPos.y()),
            qAbs(m_endPos.x() - m_startPos.x()),
            qAbs(m_endPos.y() - m_startPos.y())
        );
        
        if (m_selectedArea.width() < 5 || m_selectedArea.height() < 5) {
            m_selectedArea = QRect();
        }
        
        update();
    }
}

void ScreenshotTool::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        // 如果正在自定义尺寸，先退出自定义模式
        if (m_isCustomSizing) {
            m_isCustomSizing = false;
            hideCustomSizeControls();
            update();
        } else {
            // 否则直接取消截图
            close();
        }
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // 确认截图
        if (!m_selectedArea.isEmpty()) {
            takeScreenshot();
            saveScreenshot();
            copyToClipboard();
            close();
        }
    } else if (event->key() == Qt::Key_S && (event->modifiers() & Qt::ControlModifier)) {
        // Ctrl+S 调出自定义尺寸对话框
        if (!m_selectedArea.isEmpty()) {
            // 如果已有选区，填入当前尺寸
            m_widthEdit->setText(QString::number(m_selectedArea.width()));
            m_heightEdit->setText(QString::number(m_selectedArea.height()));
        } else {
            // 否则使用默认尺寸
            m_widthEdit->setText("800");
            m_heightEdit->setText("600");
        }
        showSizeInputDialog();
    }
}

void ScreenshotTool::takeScreenshot()
{
    if (!m_selectedArea.isEmpty()) {
        m_selectedPixmap = m_fullScreenPixmap.copy(m_selectedArea);
    }
}

void ScreenshotTool::saveScreenshot()
{
    if (m_selectedPixmap.isNull()) {
        return;
    }
    
    QString fileName = generateFileName();
    m_selectedPixmap.save(fileName, "PNG");
}

void ScreenshotTool::copyToClipboard()
{
    if (m_selectedPixmap.isNull()) {
        return;
    }
    
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setPixmap(m_selectedPixmap);
}

void ScreenshotTool::resetSelection()
{
    m_isSelecting = false;
    m_isCustomSizing = false;
    m_startPos = QPoint();
    m_endPos = QPoint();
    m_selectedArea = QRect();
    m_selectedPixmap = QPixmap();
    hideCustomSizeControls();
}

QString ScreenshotTool::generateFileName() const
{
    QString dateTime = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    return QString("./screenshot_%1.png").arg(dateTime);
}

void ScreenshotTool::showSizeInputDialog()
{
    // 居中显示输入对话框
    QRect screenGeometry = geometry();
    int x = (screenGeometry.width() - m_customSizeWidget->width()) / 2;
    int y = (screenGeometry.height() - m_customSizeWidget->height()) / 2;
    m_customSizeWidget->move(x, y);
    
    m_isCustomSizing = true;
    m_customSizeWidget->show();
    m_widthEdit->setFocus();
    
    // 如果已有自定义选区，则更新
    updateSelectedAreaFromInputs();
}

void ScreenshotTool::hideCustomSizeControls()
{
    m_customSizeWidget->hide();
}

void ScreenshotTool::onWidthChanged(const QString &text)
{
    if (m_isCustomSizing && !text.isEmpty()) {
        updateSelectedAreaFromInputs();
    }
}

void ScreenshotTool::onHeightChanged(const QString &text)
{
    if (m_isCustomSizing && !text.isEmpty()) {
        updateSelectedAreaFromInputs();
    }
}

void ScreenshotTool::updateSelectedAreaFromInputs()
{
    bool widthOk = false, heightOk = false;
    int width = m_widthEdit->text().toInt(&widthOk);
    int height = m_heightEdit->text().toInt(&heightOk);
    
    if (widthOk && heightOk && width > 0 && height > 0) {
        // 获取屏幕中心点
        QRect screenGeometry = geometry();
        int centerX = screenGeometry.width() / 2;
        int centerY = screenGeometry.height() / 2;
        
        // 计算选区左上角
        int left = centerX - width / 2;
        int top = centerY - height / 2;
        
        // 确保选区在屏幕范围内
        left = qMax(0, left);
        top = qMax(0, top);
        
        if (left + width > screenGeometry.width()) {
            width = screenGeometry.width() - left;
        }
        
        if (top + height > screenGeometry.height()) {
            height = screenGeometry.height() - top;
        }
        
        // 更新选区
        m_selectedArea = QRect(left, top, width, height);
        update();
    }
}

void ScreenshotTool::onCustomSizeConfirmed()
{
    // 应用自定义尺寸
    updateSelectedAreaFromInputs();
    m_isCustomSizing = false;
    hideCustomSizeControls();
}

void ScreenshotTool::onCustomSizeCancelled()
{
    // 取消自定义尺寸
    m_isCustomSizing = false;
    hideCustomSizeControls();
    update();
} 