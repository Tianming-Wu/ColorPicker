#pragma once

#include <QWidget>
#include <QColor>
#include <QPainter>

class ColorPreview : public QWidget {
    Q_OBJECT
public:
    explicit ColorPreview(QWidget *parent = nullptr)
        : QWidget(parent), checker(16, 16)
    {
        checker.fill(Qt::white);
        QPainter p(&checker);
        p.fillRect(0, 0, 8, 8, QColor(200, 200, 200));
        p.fillRect(8, 8, 8, 8, QColor(200, 200, 200));
    }
    void setColor(const QColor &color) {
        m_color = color;
        update();
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setBrush(QBrush(checker));
        p.drawRect(rect());
        p.fillRect(rect(), m_color);
    }
private:
    QColor m_color;
    QPixmap checker;
};

class ColorPicker : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool alphaEnabled READ alphaEnabled WRITE setAlphaEnabled NOTIFY alphaEnabledChanged)

public:
    explicit ColorPicker(QWidget *parent = nullptr);

    QColor color() const;
    bool alphaEnabled() const { return m_alphaEnabled; }
    QString colorRgbString() const;
    QString colorHsvString() const;
    QString colorQColorString() const;

signals:
    void colorChanged(const QColor &color);
    void alphaEnabledChanged(bool enabled);

public slots:
    void setColor(const QColor &color);
    void setAlphaEnabled(bool enabled);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool event(QEvent *event) override;

private:
    void updateHsvFromHueSatPos(const QPoint &pos);
    void updateHsvFromValuePos(int y);
    void updateAlphaFromPos(int y);
    void updateColor();

    QColor m_currentColor;
    QImage m_hueSatImage;
    QImage m_valueImage;
    QImage m_alphaImage;
    QPixmap m_hueSatPixmap;
    QPixmap m_valuePixmap;
    QPixmap m_alphaPixmap;
    bool m_mouseDown = false;
    bool m_alphaEnabled = false;
    int m_valueWidth = 30;
    int m_alphaWidth = 30;
    int m_spacing = 10;

    QPixmap checker;

    enum InteractionArea {
        AreaNone, AreaHueSat, AreaValue, AreaAlpha
    } m_interactionArea = AreaNone;
};
