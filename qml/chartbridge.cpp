// chartbridge.cpp
#include "chartbridge.h"

ChartBridge::ChartBridge(QObject* parent) : QObject(parent)
{
    // ~60 FPS GUI updates; adjust if you want 30/120 Hz
    m_flush.setInterval(16);
    connect(&m_flush, &QTimer::timeout, this, &ChartBridge::flush);
    m_flush.start();

    // Optional: pre-reserve to reduce reallocations
    m_data.reserve(200000);
}

void ChartBridge::setSeries(QXYSeries* series)
{
    m_series = series;
    // initial clear
    if (m_series)
        m_series->replace({});
}

void ChartBridge::setMaxPoints(int n)
{
    if (n == 0) n = -1; // treat 0 as unlimited
    if (m_maxPoints == n) return;
    m_maxPoints = n;
    emit maxPointsChanged();

    // Trim immediately if the new cap is lower than current size
    if (m_maxPoints > 0 && m_data.size() > m_maxPoints) {
        m_data.erase(m_data.begin(), m_data.end() - m_maxPoints);
        if (m_series)
            m_series->replace(m_data);
    }
}

void ChartBridge::onBatch(const QVector<QPointF>& pts)
{
    if (pts.isEmpty()) return;
    QMutexLocker lk(&m_mx);
    m_buffer += pts; // accumulate all points received since last flush
}

void ChartBridge::reset()
{
    {
        QMutexLocker lk(&m_mx);
        m_buffer.clear();
        m_data.clear();
    }
    if (m_series)
        m_series->replace({});
}

void ChartBridge::flush()
{
    QVector<QPointF> local;
    {
        QMutexLocker lk(&m_mx);
        if (m_buffer.isEmpty())
            return;
        local.swap(m_buffer); // take everything pending
    }

    // Append to persistent history
    m_data += local;

    // Optional cap to avoid unbounded memory growth
    if (m_maxPoints > 0 && m_data.size() > m_maxPoints)
        m_data.erase(m_data.begin(), m_data.end() - m_maxPoints);

    // Bulk update the series with the entire history
    if (m_series)
        m_series->replace(m_data);
}
