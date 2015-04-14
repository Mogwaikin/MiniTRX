
#include <QtWidgets/QLabel>
#include <QtGui/QMouseEvent>

#include "indicator.h"

//------------------------------------------------------------------------------

static const QString c_StyleSheets[3] =
{
  "",
  "background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(255, 153, 153, 255), stop:0.49 rgba(255, 153, 153, 255), stop:0.50 rgba(0, 0, 0, 0), stop:1 rgba(0, 0, 0, 0))",
  "background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(0, 0, 0, 0), stop:0.50 rgba(0, 0, 0, 0), stop:0.51 rgba(153, 204, 255, 255), stop:1 rgba(153, 204, 255, 255))"
};

//------------------------------------------------------------------------------

class CustomDigit: public QLabel
{
public:
  CustomDigit(QWidget *parent = 0): QLabel(parent), m_Active(true), m_State(0), m_Delta(0) {}

  void mouseMoveEvent(QMouseEvent *event)
  {
    if(!m_Active) return;
    QPalette pal;
    QColor color = palette().color(QPalette::WindowText);
    if(event->y() < height()/2 && m_State != 1)
    {
      m_State = 1;
      setStyleSheet(c_StyleSheets[1]);
      pal = palette();
      pal.setColor(QPalette::WindowText, color);
      setPalette(pal);
    }
    if(event->y() > height()/2 && m_State != 2)
    {
      m_State = 2;
      setStyleSheet(c_StyleSheets[2]);
      pal = palette();
      pal.setColor(QPalette::WindowText, color);
      setPalette(pal);
    }
  }

  void leaveEvent(QEvent *event)
  {
    if(!m_Active) return;
    QPalette pal;
    QColor color = palette().color(QPalette::WindowText);
    m_State = 0;
    setStyleSheet(c_StyleSheets[0]);
    pal = palette();
    pal.setColor(QPalette::WindowText, color);
    setPalette(pal);
  }

  void mousePressEvent(QMouseEvent *event)
  {
    if(!m_Active) return;
    if(event->y() < height()/2)
    {
      m_Indicator->applyDelta(m_Delta);
    }
    if(event->y() > height()/2)
    {
      m_Indicator->applyDelta(-m_Delta);
    }
  }

  void wheelEvent(QWheelEvent *event)
  {
    if(!m_Active) return;
    m_Indicator->applyDelta(event->delta()/90*m_Delta);
  }

  bool m_Active;
  int m_State;
  int m_Delta;
  Indicator *m_Indicator;
};

//------------------------------------------------------------------------------

Indicator::Indicator(QWidget *parent):
  QFrame(parent)
{
  CustomDigit *digit;
  int i, x, delta;
  QFont font("Arial", 21, QFont::Normal);
  x = 143;
  delta = 1;
  for(i = 0; i < 8; ++i)
  {
    digit = new CustomDigit(this);
    digit->setText(QString::number(0));
    digit->setAlignment(Qt::AlignCenter);
    digit->setFont(font);
    digit->setGeometry(QRect(x, 2, 18, 27));
    digit->setMouseTracking(true);
    digit->m_Delta = delta;
    digit->m_Indicator = this;
    x -= (i % 3 == 2) ? 23 : 18;
    delta *= 10;
  }
}

//------------------------------------------------------------------------------

Indicator::~Indicator()
{
}

//------------------------------------------------------------------------------

void Indicator::setValue(int value)
{
  int quotient;
  QPalette palette;
  if(value < 0 && value > 50000000) return;
  foreach(CustomDigit *digit, findChildren<CustomDigit *>())
  {
    quotient = value/digit->m_Delta;
    palette = digit->palette();
    if(quotient == 0)
    {
      palette.setColor(QPalette::WindowText, QColor(Qt::lightGray));
    }
    else
    {
      palette.setColor(QPalette::WindowText, QColor(Qt::black));
    }
    digit->setPalette(palette);
    digit->setText(QString::number(quotient%10));
  }
  m_Value = value;
  emit valueChanged(value);
}

//------------------------------------------------------------------------------

void Indicator::setDeltaMin(int delta)
{
  foreach(CustomDigit *digit, findChildren<CustomDigit *>())
  {
    if(digit->m_Delta < delta)
    {
      digit->setText(QString::number(0));
      digit->setMouseTracking(false);
      digit->m_Active = false;
    }
    else
    {
      digit->setMouseTracking(true);
      digit->m_Active = true;
    }
  }
}

//------------------------------------------------------------------------------

void Indicator::applyDelta(int delta)
{
  int value = m_Value + delta;
  if(delta < 0 && value < 0) value = 0;
  if(delta > 0 && value > 50000000) value = 50000000;
  setValue(value);
}
