#ifndef CHARTBRIDGE_H
#define CHARTBRIDGE_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QVector>
#include <QPointF>
#include <QtCharts/QXYSeries>

class ChartBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int maxPoints READ maxPoints WRITE setMaxPoints NOTIFY maxPointsChanged)
public:
    explicit ChartBridge(QObject* parent=nullptr);

    Q_INVOKABLE void setSeries(QXYSeries* series); // call from QML on Component.onCompleted

    int  maxPoints() const { return m_maxPoints; }
    void setMaxPoints(int n);

public slots:
    void onBatch(const QVector<QPointF>& pts); // from worker
    void reset();                              // clear series

signals:
    void maxPointsChanged();

private slots:
    void flush(); // 60 Hz

private:
    QXYSeries* m_series = nullptr;   // GUI thread only
    QVector<QPointF> m_buffer;                 // guarded by m_mx
    QVector<QPointF> m_data;                   // GUI thread
    QMutex m_mx;
    QTimer m_flush;
    int    m_maxPoints = -1;                 // sliding window size
};

#endif // CHARTBRIDGE_H
