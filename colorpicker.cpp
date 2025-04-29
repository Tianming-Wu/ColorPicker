#include "colorpicker.h"

#include <QPainter>
#include <QEvent>
#include <QMouseEvent>
#include <QLinearGradient>
#include <QToolTip>

ColorPicker::ColorPicker(QWidget *parent)
    : QWidget(parent), checker(16, 16)
{
    setColor(Qt::red);
    setMinimumSize(300, 200);

    checker.fill(Qt::white);
    QPainter p(&checker);
    p.fillRect(0, 0, 8, 8, QColor(200, 200, 200));
    p.fillRect(8, 8, 8, 8, QColor(200, 200, 200));
}

QColor ColorPicker::color() const
{
    return m_currentColor;
}

void ColorPicker::setColor(const QColor &color)
{
    if (m_currentColor == color)
        return;

    m_currentColor = color;
    update();
    emit colorChanged(color);
}

QString ColorPicker::colorRgbString() const
{
    if (m_alphaEnabled) {
        return QString("rgba(%1,%2,%3,%4)")
        .arg(m_currentColor.red())
            .arg(m_currentColor.green())
            .arg(m_currentColor.blue())
            .arg(m_currentColor.alphaF(), 0, 'f', 2);
    }
    return QString("rgb(%1,%2,%3)")
        .arg(m_currentColor.red())
        .arg(m_currentColor.green())
        .arg(m_currentColor.blue());
}

QString ColorPicker::colorHsvString() const
{
    if (m_alphaEnabled) {
        return QString("hsva(%1,%2,%3,%4)")
        .arg(m_currentColor.hsvHue())
            .arg(qRound(m_currentColor.hsvSaturationF() * 100))
            .arg(qRound(m_currentColor.valueF() * 100))
            .arg(m_currentColor.alphaF(), 0, 'f', 2);
    }
    return QString("hsv(%1,%2,%3)")
        .arg(m_currentColor.hsvHue())
        .arg(qRound(m_currentColor.hsvSaturationF() * 100))
        .arg(qRound(m_currentColor.valueF() * 100));
}

QString ColorPicker::colorQColorString() const
{
    if (m_alphaEnabled) {
        return QString("QColor(%1,%2,%3,%4)")
        .arg(m_currentColor.red())
            .arg(m_currentColor.green())
            .arg(m_currentColor.blue())
            .arg(m_currentColor.alpha());
    }
    return QString("QColor(%1,%2,%3)")
        .arg(m_currentColor.red())
        .arg(m_currentColor.green())
        .arg(m_currentColor.blue());
}

void ColorPicker::setAlphaEnabled(bool enabled)
{
    // 在关闭 Alpha 时，保留当前颜色的视觉表现（忽略 Alpha 通道）
    QColor newColor = m_currentColor;
    if (!enabled) {
        newColor.setAlphaF(1.0); // 强制不透明
    } else {
        newColor.setAlphaF(m_currentColor.alphaF()); // 恢复之前的 Alpha
    }

    m_alphaEnabled = enabled;
    resizeEvent(nullptr);
    setColor(newColor); // 触发颜色更新和重绘
    emit alphaEnabledChanged(enabled);
    emit colorChanged(m_currentColor);
}

void ColorPicker::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 计算各区域位置（与resizeEvent/mousePressEvent一致）
    int hueSatWidth = width() - m_valueWidth - m_spacing - (m_alphaEnabled ? m_alphaWidth + m_spacing : 0);
    int valueBarX = hueSatWidth + m_spacing;
    int alphaBarX = width() - m_alphaWidth;

    // 1. 绘制色相-饱和度区域
    painter.drawPixmap(0, 0, m_hueSatPixmap);

    // 2. 绘制明度条
    painter.drawPixmap(valueBarX, 0, m_valuePixmap);

    // 3. 绘制透明度条（如果启用）
    if (m_alphaEnabled) {
        painter.drawPixmap(alphaBarX, 0, m_alphaPixmap);

    }

    // === 绘制选择光标 ===
    // 1. 色相-饱和度区域的圆圈
    QPoint hueSatPos(
        m_currentColor.hsvHueF() * hueSatWidth,
        (1 - m_currentColor.hsvSaturationF()) * height()
        );
    painter.setPen(QPen(Qt::white, 2));
    painter.drawEllipse(hueSatPos, 6, 6);
    painter.setPen(QPen(Qt::black, 1));
    painter.drawEllipse(hueSatPos, 6, 6);

    // 2. 明度条的指示线
    int valuePos = (1 - m_currentColor.valueF()) * height();
    painter.setPen(QPen(Qt::white, 2));
    painter.drawLine(valueBarX - 3, valuePos, valueBarX + m_valueWidth + 2, valuePos);

    // 3. 透明度条的指示线（如果启用）
    if (m_alphaEnabled) {
        int alphaPos = (1 - m_currentColor.alphaF()) * height();
        painter.drawLine(alphaBarX - 3, alphaPos, alphaBarX + m_alphaWidth + 2, alphaPos);
    }
}

void ColorPicker::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QPoint pos = event->pos();
        int hueSatWidth = width() - m_valueWidth - m_spacing - (m_alphaEnabled ? m_alphaWidth + m_spacing : 0);

        if (pos.x() < hueSatWidth) {
            m_interactionArea = AreaHueSat;
            updateHsvFromHueSatPos(pos);
        }
        else if (pos.x() < hueSatWidth + m_spacing) {
            m_interactionArea = AreaNone; // 点击在间隔区域，忽略
        }
        else if (pos.x() < hueSatWidth + m_spacing + m_valueWidth) {
            m_interactionArea = AreaValue;
            updateHsvFromValuePos(pos.y());
        }
        else if (m_alphaEnabled && pos.x() >= width() - m_alphaWidth) {
            m_interactionArea = AreaAlpha;
            updateAlphaFromPos(pos.y());
        }
        else {
            m_interactionArea = AreaNone;
        }
        m_mouseDown = true;
    }
}

void ColorPicker::mouseMoveEvent(QMouseEvent *event)
{
    if (m_mouseDown) {
        QPoint pos = event->pos();

        switch (m_interactionArea) {
        case AreaHueSat: {
            int hueSatWidth = width() - m_valueWidth - (m_alphaEnabled ? m_alphaWidth + m_spacing * 2 : m_spacing);
            pos.setX(qBound(0, pos.x(), hueSatWidth - 1));
            pos.setY(qBound(0, pos.y(), height() - 1));
            updateHsvFromHueSatPos(pos);
            break;
        }
        case AreaValue: {
            pos.setY(qBound(0, pos.y(), height() - 1));
            updateHsvFromValuePos(pos.y());
            break;
        }
        case AreaAlpha: {
            pos.setY(qBound(0, pos.y(), height() - 1));
            updateAlphaFromPos(pos.y());
            break;
        }
        case AreaNone:
            // 忽略中间空白区域的拖动
            break;
        }
    }
}

void ColorPicker::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_mouseDown = false;
        m_interactionArea = AreaNone;
    }
}

void ColorPicker::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);

    // 计算色相-饱和度区域宽度（始终保留 spacing）
    int hueSatWidth = width() - m_valueWidth - m_spacing - (m_alphaEnabled ? m_alphaWidth + m_spacing : 0);

    // 生成色相-饱和度区域（始终不透明，Alpha 仅影响最终颜色）
    m_hueSatImage = QImage(hueSatWidth, height(), QImage::Format_ARGB32);
    for (int y = 0; y < height(); ++y) {
        for (int x = 0; x < hueSatWidth; ++x) {
            qreal hue = qreal(x) / hueSatWidth;
            qreal sat = 1.0 - qreal(y) / height();
            QColor color = QColor::fromHsvF(hue, sat, 1.0); // 固定明度 1.0，确保亮度一致
            m_hueSatImage.setPixel(x, y, color.rgb()); // 使用 rgb() 忽略 Alpha
        }
    }
    m_hueSatPixmap = QPixmap::fromImage(m_hueSatImage);

    // 生成明度条图像（始终位于 hueSatWidth + spacing 处）
    m_valueImage = QImage(m_valueWidth, height(), QImage::Format_ARGB32);
    QLinearGradient gradient(0, 0, 0, height());
    gradient.setColorAt(0.0, QColor::fromHsvF(m_currentColor.hsvHueF(), m_currentColor.hsvSaturationF(), 1.0));
    gradient.setColorAt(1.0, QColor::fromHsvF(m_currentColor.hsvHueF(), m_currentColor.hsvSaturationF(), 0.0));
    if (m_alphaEnabled) {
        gradient.setColorAt(0.0, QColor::fromHsvF(m_currentColor.hsvHueF(), m_currentColor.hsvSaturationF(), 1.0, m_currentColor.alphaF()));
        gradient.setColorAt(1.0, QColor::fromHsvF(m_currentColor.hsvHueF(), m_currentColor.hsvSaturationF(), 0.0, m_currentColor.alphaF()));
    }
    QPainter painter(&m_valueImage);
    painter.fillRect(m_valueImage.rect(), gradient);
    m_valuePixmap = QPixmap::fromImage(m_valueImage);

    if (m_alphaEnabled) {
        m_alphaImage = QImage(m_alphaWidth, height(), QImage::Format_ARGB32);
        m_alphaImage.fill(Qt::transparent);

        QPainter alphaPainter(&m_alphaImage);

        // 先绘制棋盘格背景（平铺）
        alphaPainter.setBrush(QBrush(checker));
        alphaPainter.drawRect(0, 0, m_alphaWidth, height());

        // 再绘制透明度覆盖层（从顶部不透明到底部透明）
        QLinearGradient alphaGradient(0, 0, 0, height());
        QColor baseColor = QColor::fromHsvF(
            m_currentColor.hsvHueF(),
            m_currentColor.hsvSaturationF(),
            m_currentColor.valueF()
            );
        alphaGradient.setColorAt(0.0, baseColor); // 顶部完全不透明
        alphaGradient.setColorAt(1.0, Qt::transparent); // 底部完全透明

        alphaPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        alphaPainter.fillRect(0, 0, m_alphaWidth, height(), alphaGradient);

        m_alphaPixmap = QPixmap::fromImage(m_alphaImage);
    }


    update();
}

void ColorPicker::updateHsvFromHueSatPos(const QPoint &pos)
{
    // 计算色相-饱和度区域宽度（与resizeEvent一致）
    int hueSatWidth = width() - m_valueWidth - m_spacing - (m_alphaEnabled ? m_alphaWidth + m_spacing : 0);

    // 获取当前鼠标位置对应的HSV值
    qreal hue = qBound(0.0, qreal(pos.x()) / hueSatWidth, 1.0);
    qreal sat = qBound(0.0, 1.0 - qreal(pos.y()) / height(), 1.0);

    QColor newColor = QColor::fromHsvF(
        hue,
        sat,
        m_currentColor.valueF(), // 保留原有明度
        m_alphaEnabled ? m_currentColor.alphaF() : 1.0
        );

    if (newColor != m_currentColor) {
        m_currentColor = newColor;

        // === 关键修复：更新明度条渐变 ===
        m_valueImage = QImage(m_valueWidth, height(), QImage::Format_ARGB32);
        QLinearGradient gradient(0, 0, 0, height());
        gradient.setColorAt(0.0, QColor::fromHsvF(hue, sat, 1.0));      // 最亮
        gradient.setColorAt(1.0, QColor::fromHsvF(hue, sat, 0.0));      // 最暗
        if (m_alphaEnabled) {
            gradient.setColorAt(0.0, QColor::fromHsvF(hue, sat, 1.0, m_currentColor.alphaF()));
            gradient.setColorAt(1.0, QColor::fromHsvF(hue, sat, 0.0, m_currentColor.alphaF()));
        }

        QPainter painter(&m_valueImage);
        painter.fillRect(m_valueImage.rect(), gradient);
        m_valuePixmap = QPixmap::fromImage(m_valueImage);

        // === 如果启用透明度，也更新透明度条 ===
        // if (m_alphaEnabled) {
        //     m_alphaImage = QImage(m_alphaWidth, height(), QImage::Format_ARGB32);
        //     QLinearGradient alphaGradient(0, 0, 0, height());
        //     alphaGradient.setColorAt(0.0, QColor::fromHsvF(hue, sat, m_currentColor.valueF(), 1.0));
        //     alphaGradient.setColorAt(1.0, QColor::fromHsvF(hue, sat, m_currentColor.valueF(), 0.0));

        //     QPainter alphaPainter(&m_alphaImage);
        //     alphaPainter.fillRect(m_alphaImage.rect(), alphaGradient);
        //     m_alphaPixmap = QPixmap::fromImage(m_alphaImage);
        // }

        if (m_alphaEnabled) {
            m_alphaImage = QImage(m_alphaWidth, height(), QImage::Format_ARGB32);
            m_alphaImage.fill(Qt::transparent);

            QPainter alphaPainter(&m_alphaImage);

            alphaPainter.setBrush(QBrush(checker));
            alphaPainter.drawRect(0, 0, m_alphaWidth, height());

            // 再绘制透明度覆盖层（从顶部不透明到底部透明）
            QLinearGradient alphaGradient(0, 0, 0, height());
            QColor baseColor = QColor::fromHsvF(
                m_currentColor.hsvHueF(),
                m_currentColor.hsvSaturationF(),
                m_currentColor.valueF()
                );
            alphaGradient.setColorAt(0.0, baseColor); // 顶部完全不透明
            alphaGradient.setColorAt(1.0, Qt::transparent); // 底部完全透明

            alphaPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            alphaPainter.fillRect(0, 0, m_alphaWidth, height(), alphaGradient);

            m_alphaPixmap = QPixmap::fromImage(m_alphaImage);
        }

        update(); // 触发重绘
        emit colorChanged(m_currentColor);
    }
}

void ColorPicker::updateHsvFromValuePos(int y)
{
    qreal value = qBound(0.0, 1.0 - qreal(y) / height(), 1.0);
    QColor newColor = QColor::fromHsvF(
        m_currentColor.hsvHueF(),
        m_currentColor.hsvSaturationF(),
        value,
        m_alphaEnabled ? m_currentColor.alphaF() : 1.0  // 继承当前 Alpha
        );

    if (m_alphaEnabled) {
        m_alphaImage = QImage(m_alphaWidth, height(), QImage::Format_ARGB32);
        m_alphaImage.fill(Qt::transparent);

        QPainter alphaPainter(&m_alphaImage);

        // 先绘制棋盘格背景（平铺）
        alphaPainter.setBrush(QBrush(checker));
        alphaPainter.drawRect(0, 0, m_alphaWidth, height());

        // 再绘制透明度覆盖层（从顶部不透明到底部透明）
        QLinearGradient alphaGradient(0, 0, 0, height());
        QColor baseColor = QColor::fromHsvF(
            m_currentColor.hsvHueF(),
            m_currentColor.hsvSaturationF(),
            m_currentColor.valueF()
            );
        alphaGradient.setColorAt(0.0, baseColor); // 顶部完全不透明
        alphaGradient.setColorAt(1.0, Qt::transparent); // 底部完全透明

        alphaPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        alphaPainter.fillRect(0, 0, m_alphaWidth, height(), alphaGradient);

        m_alphaPixmap = QPixmap::fromImage(m_alphaImage);
    }


    setColor(newColor);
}

void ColorPicker::updateAlphaFromPos(int y)
{
    qreal alpha = qBound(0.0, 1.0 - qreal(y) / height(), 1.0);
    QColor newColor = m_currentColor;
    newColor.setAlphaF(alpha);

    if (newColor != m_currentColor) {
        m_currentColor = newColor;
        update();
        emit colorChanged(m_currentColor);
    }
}

bool ColorPicker::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        QToolTip::showText(helpEvent->globalPos(),
                           QString("%1\n%2").arg(colorRgbString()).arg(colorHsvString()));
        return true;
    }
    return QWidget::event(event);
}
