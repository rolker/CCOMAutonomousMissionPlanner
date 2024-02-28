#ifndef PROJECTVIEW_H
#define PROJECTVIEW_H

#include <QGraphicsView>
#include <QGeoCoordinate>

class QStatusBar;
class QLabel;
class AutonomousVehicleProject;
class TrackLine;
class SurveyPattern;
class SurveyArea;
class SearchPattern;
class Waypoint;
class BackgroundRaster;
class MeasuringTool;
class AvoidArea;

class ProjectView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit ProjectView(QWidget *parent = 0);
    void setStatusBar(QStatusBar *bar);
    void setAddWaypointMode();
    void setAddTracklineMode();
    void setAddSurveyPatternMode();
    void setAddSurveyAreaMode();
    void setAddSearchPatternMode();
    void setAddAvoidAreaMode();
    void setPanMode();
    void setProject(AutonomousVehicleProject *project);

    bool isMouseModeAddAvoidArea() const;

signals:
    void currentChanged(QModelIndex &index);
    void scaleChanged(qreal scale);
    void viewportChanged(QPointF ll, QPointF ur);

public slots:
    void sendHover();
    void sendGoto();
    void sendIdle();
    void sendLookAt();
    void sendLookAtASV();
    void beforeUpdateBackground();
    void updateBackground(BackgroundRaster * bg);
    void sendViewport(); 
    void centerMap(QGeoCoordinate location);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    enum class MouseMode {pan, addWaypoint, addTrackline, addSurveyPattern, addSurveyArea, addSearchPattern, addAvoidArea};
    QStatusBar * statusBar;
    QLabel * positionLabel;
    QLabel * modeLabel;
    MouseMode mouseMode;
    AutonomousVehicleProject *m_project;
    TrackLine * currentTrackLine;
    Waypoint * pendingTrackLineWaypoint = nullptr;
    SurveyPattern * pendingSurveyPattern = nullptr;
    SurveyArea * pendingSurveyArea = nullptr;
    AvoidArea * pendingAvoidArea = nullptr;
    SearchPattern * pendingSearchPattern = nullptr;
    Waypoint * pendingSurveyAreaWaypoint = nullptr;
    Waypoint * pendingAvoidAreaWaypoint = nullptr;
    MeasuringTool * measuringTool;

    QGeoCoordinate m_contextMenuLocation;

    QGeoCoordinate m_savedCenter;

};

#endif // PROJECTVIEW_H
