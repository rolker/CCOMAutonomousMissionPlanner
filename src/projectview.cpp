#include "projectview.h"

#include <QWheelEvent>
#include <QLabel>
#include <QStatusBar>
#include <QStandardItemModel>
#include "autonomousvehicleproject.h"
#include "backgroundraster.h"
#include "waypoint.h"
#include "trackline.h"
#include "surveypattern.h"
#include "surveyarea.h"
#include "searchpattern.h"
#include "avoid_area.h"
#include <QDebug>
#include <QMenu>
#include "measuringtool.h"
#include <QAbstractSlider>
#include <QScrollBar>
#include "roslink.h"
#include "platform_manager/platform.h"
#include "mission_manager/mission_manager.h"
#include "helm_manager/helm_manager.h"


ProjectView::ProjectView(QWidget *parent) : QGraphicsView(parent),
    statusBar(0), positionLabel(new QLabel()), modeLabel(new QLabel()), mouseMode(MouseMode::pan), currentTrackLine(nullptr), pendingTrackLineWaypoint(nullptr), pendingSurveyPattern(nullptr), pendingSurveyArea(nullptr),pendingSurveyAreaWaypoint(nullptr),measuringTool(nullptr)
{

    positionLabel->setText("(,)");
    modeLabel->setText("Mode: pan");

    connect(horizontalScrollBar(), &QAbstractSlider::valueChanged, this, &ProjectView::sendViewport);
    connect(verticalScrollBar(), &QAbstractSlider::valueChanged, this , &ProjectView::sendViewport);
}

void ProjectView::wheelEvent(QWheelEvent *event)
{
    if(event->angleDelta().y()<0)
        scale(.8,.8);
    if(event->angleDelta().y()>0)
        scale(1.25,1.25);
    emit scaleChanged(matrix().m11());
    sendViewport();
    event->accept();
}

void ProjectView::mousePressEvent(QMouseEvent *event)
{
    BackgroundRaster *bg =  m_project->getBackgroundRaster();
    switch(event->button())
    {
    case Qt::LeftButton:
        switch(mouseMode)
        {
        case MouseMode::pan:
            break;
        case MouseMode::addWaypoint:
            if(bg)
            {
                m_project->addWaypoint(bg->pixelToGeo(mapToScene(event->pos())));
            }
            setPanMode();
            break;
        case MouseMode::addTrackline:
            if(!currentTrackLine)
            {
                if(bg)
                {
                    currentTrackLine = m_project->addTrackLine(bg->pixelToGeo(mapToScene(event->pos())));
                    pendingTrackLineWaypoint = currentTrackLine->addWaypoint(bg->pixelToGeo(mapToScene(event->pos())));
                }
            }
            else
            {
                pendingTrackLineWaypoint = currentTrackLine->addWaypoint(bg->pixelToGeo(mapToScene(event->pos())));

            }
            break;
        case MouseMode::addSurveyPattern:
            if(!pendingSurveyPattern)
            {
                if(bg)
                {
                    pendingSurveyPattern = m_project->addSurveyPattern(bg->pixelToGeo(mapToScene(event->pos())));
                    //QModelIndex i = m_project-> indexFromItem(pendingSurveyPattern);
                    //emit  currentChanged(i);
                }
            }
            else
            {
                if(pendingSurveyPattern->hasSpacingLocation())
                {
                    setPanMode();
                }
                else
                {
                    pendingSurveyPattern->setSpacingLocation(bg->pixelToGeo(mapToScene(event->pos())));
                }
            }
            break;
        case MouseMode::addSurveyArea:
            if(!pendingSurveyArea)
            {
                if(bg)
                {
                    pendingSurveyArea = m_project->addSurveyArea(bg->pixelToGeo(mapToScene(event->pos())));
                    pendingSurveyAreaWaypoint = pendingSurveyArea->addWaypoint(bg->pixelToGeo(mapToScene(event->pos())));
                }
            }
            else
            {
                pendingSurveyAreaWaypoint = pendingSurveyArea->addWaypoint(bg->pixelToGeo(mapToScene(event->pos())));
            }
            break;
        case MouseMode::addAvoidArea:
            if(!pendingAvoidArea)
            {
                if(bg)
                {
                    pendingAvoidArea = m_project->addAvoidArea(bg->pixelToGeo(mapToScene(event->pos())));
                    pendingAvoidAreaWaypoint = pendingAvoidArea->addPoint(bg->pixelToGeo(mapToScene(event->pos())));
                }
            }
            else
            {
                pendingAvoidAreaWaypoint = pendingAvoidArea->addPoint(bg->pixelToGeo(mapToScene(event->pos())));
            }
            break;
        case MouseMode::addSearchPattern:
            if(!pendingSearchPattern)
            {
                if(bg)
                {
                    pendingSearchPattern = m_project->addSearchPattern(bg->pixelToGeo(mapToScene(event->pos())));
                    //QModelIndex i = m_project-> indexFromItem(pendingSurveyPattern);
                    //emit  currentChanged(i);
                }
            }
            else
            {
                if(pendingSearchPattern->hasSpacingLocation())
                {
                    setPanMode();
                }
                else
                {
                    pendingSearchPattern->setSpacingLocation(bg->pixelToGeo(mapToScene(event->pos())));
                }
            }
            break;
        }
        break;

    case Qt::RightButton:
        if(mouseMode == MouseMode::addTrackline || mouseMode == MouseMode::addWaypoint || mouseMode == MouseMode::addSurveyPattern || mouseMode == MouseMode::addSurveyArea || mouseMode == MouseMode::addSearchPattern || mouseMode == MouseMode::addAvoidArea)
        {
            if(mouseMode == MouseMode::addTrackline && currentTrackLine)
            {
                m_project->scene()->removeItem(pendingTrackLineWaypoint);
                m_project->deleteItem(pendingTrackLineWaypoint);
                m_project->scene()->update();
                pendingTrackLineWaypoint = nullptr;
                update();
            }
            if(mouseMode == MouseMode::addSurveyArea && pendingSurveyArea)
            {
                m_project->scene()->removeItem(pendingSurveyAreaWaypoint);
                m_project->deleteItem(pendingSurveyAreaWaypoint);
                m_project->scene()->update();
                pendingSurveyAreaWaypoint = nullptr;
                pendingSurveyArea = nullptr;
                update();
            }
            if(mouseMode == MouseMode::addAvoidArea && pendingAvoidArea)
            {
                m_project->scene()->removeItem(pendingAvoidAreaWaypoint);
                m_project->deleteItem(pendingAvoidAreaWaypoint);
                m_project->scene()->update();
                pendingAvoidAreaWaypoint = nullptr;
                pendingAvoidArea = nullptr;
                m_project->updateAvoidanceAreas();
                update();
            }
            setPanMode();
            event->accept();
        }
        break;
    case Qt::MiddleButton:
        if(bg && !measuringTool)
        {
            measuringTool = new MeasuringTool(bg);
            measuringTool->setStart(bg->pixelToGeo(mapToScene(event->pos())));
            measuringTool->setFinish(bg->pixelToGeo(mapToScene(event->pos())));
        }
        break;
    default:
        break;
    }
    QGraphicsView::mousePressEvent(event);
}

void ProjectView::mouseMoveEvent(QMouseEvent *event)
{
    QString posText = QString::number(event->pos().x())+","+QString::number(event->pos().y());

    QPointF transformedMouse = mapToScene(event->pos());
    BackgroundRaster *bg =  m_project->getBackgroundRaster();
    BackgroundRaster *dr =  m_project->getDepthRaster();
    if(bg)
    {
        QPointF projectedMouse = bg->pixelToProjectedPoint(transformedMouse);
        posText += " Projected mouse: "+QString::number(projectedMouse.x(),'f')+","+QString::number(projectedMouse.y(),'f');
        QGeoCoordinate llMouse = bg->unproject(projectedMouse);
        posText += " WGS84: " + llMouse.toString(QGeoCoordinate::Degrees) + " (" + llMouse.toString(QGeoCoordinate::DegreesMinutesWithHemisphere) + ")";
        
        if(dr)
        {
            if(dr == bg)
                posText += " Depth: " +QString::number(dr->getDepth(transformedMouse.x(),transformedMouse.y()));
            else 
            {
                auto p = dr->geoToPixel(llMouse);
                posText += " Depth: " +QString::number(dr->getDepth(p.x(),p.y()));
            }
        }
        
        if(pendingSurveyPattern)
        {
            if(pendingSurveyPattern->hasSpacingLocation())
                pendingSurveyPattern->setSpacingLocation(bg->pixelToGeo(mapToScene(event->pos())));
            else
                pendingSurveyPattern->setEndLocation(bg->pixelToGeo(mapToScene(event->pos())));
        }
        if(pendingTrackLineWaypoint)
        {
            pendingTrackLineWaypoint->setLocation(bg->pixelToGeo(mapToScene(event->pos())));
        }
        if(pendingSurveyAreaWaypoint)
        {
            pendingSurveyAreaWaypoint->setLocation(bg->pixelToGeo(mapToScene(event->pos())));
        }
        if(pendingAvoidAreaWaypoint)
        {
            pendingAvoidAreaWaypoint->setLocation(bg->pixelToGeo(mapToScene(event->pos())));
        }
        if(pendingSearchPattern)
        {
            if(pendingSearchPattern->hasSpacingLocation())
                pendingSearchPattern->setSpacingLocation(bg->pixelToGeo(mapToScene(event->pos())));
            else
                pendingSearchPattern->setEndLocation(bg->pixelToGeo(mapToScene(event->pos())));
        }
        if(measuringTool)
            measuringTool->setFinish(llMouse);
    }
    
    positionLabel->setText(posText);
    QGraphicsView::mouseMoveEvent(event);
}

void ProjectView::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::MiddleButton)
    {
        if(measuringTool)
            delete measuringTool;
        measuringTool = nullptr;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void ProjectView::setAddWaypointMode()
{
    setDragMode(NoDrag);
    mouseMode = MouseMode::addWaypoint;
    modeLabel->setText("Mode: add waypoint");
    setCursor(Qt::CrossCursor);
}

void ProjectView::setAddTracklineMode()
{
    setDragMode(NoDrag);
    mouseMode = MouseMode::addTrackline;
    modeLabel->setText("Mode: add trackline");
    setCursor(Qt::CrossCursor);
}

void ProjectView::setAddSurveyPatternMode()
{
    setDragMode(NoDrag);
    mouseMode = MouseMode::addSurveyPattern;
    modeLabel->setText("Mode: add survey pattern");
    setCursor(Qt::CrossCursor);
}

void ProjectView::setAddSurveyAreaMode()
{
    setDragMode(NoDrag);
    mouseMode = MouseMode::addSurveyArea;
    modeLabel->setText("Mode: add survey area");
    setCursor(Qt::CrossCursor);
}

void ProjectView::setAddAvoidAreaMode()
{
    setDragMode(NoDrag);
    mouseMode = MouseMode::addAvoidArea;
    modeLabel->setText("Mode: add area to avoid");
    setCursor(Qt::CrossCursor);
}


void ProjectView::setAddSearchPatternMode()
{
    setDragMode(NoDrag);
    mouseMode = MouseMode::addSearchPattern;
    modeLabel->setText("Mode: add search pattern");
    setCursor(Qt::CrossCursor);
}

void ProjectView::setStatusBar(QStatusBar *bar)
{
    statusBar = bar;
    statusBar->addWidget(positionLabel);
    statusBar->addPermanentWidget(modeLabel);
}

void ProjectView::setProject(AutonomousVehicleProject *project)
{
    m_project = project;
    setScene(project->scene());
}

void ProjectView::setPanMode()
{
    setDragMode(ScrollHandDrag);
    mouseMode = MouseMode::pan;
    modeLabel->setText("Mode: pan");
    unsetCursor();
    pendingSurveyPattern = nullptr;
    currentTrackLine = nullptr;
    pendingSearchPattern = nullptr;
    pendingAvoidArea = nullptr;
}

void ProjectView::contextMenuEvent(QContextMenuEvent* event)
{
    BackgroundRaster *bg = m_project->getBackgroundRaster();
    if(bg)
    {
        m_contextMenuLocation = bg->pixelToGeo(bg->mapFromParent(mapToScene(event->pos())));
        qDebug() << m_contextMenuLocation;
        QMenu menu(this);

        menu.addSeparator();
        menu.addAction("(Blank to avoid accidental hover)");
        menu.addSeparator();

        QAction *hoverAction = menu.addAction("Hover Here");
        connect(hoverAction, &QAction::triggered, this, &ProjectView::sendHover);

        QAction *gotoAction = menu.addAction("Goto Here");
        connect(gotoAction, &QAction::triggered, this, &ProjectView::sendGoto);

        QAction *idleAction = menu.addAction("Idle in place");
        connect(idleAction, &QAction::triggered, this, &ProjectView::sendIdle);

        menu.addSeparator();
        menu.addAction("(Above moves or idles boat)");
        menu.addSeparator();
        menu.addAction("(Below moves camera)");
        menu.addSeparator();
        
        
        QAction *lookAtAction = menu.addAction("Look Here");
        connect(lookAtAction, &QAction::triggered, this, &ProjectView::sendLookAt);
        
        QAction *lookAtASVAction = menu.addAction("Look at ASV");
        connect(lookAtASVAction, &QAction::triggered, this, &ProjectView::sendLookAtASV);

        menu.exec(event->globalPos());
    }
    event->accept();

}

void ProjectView::sendHover()
{
  auto p = m_project->activePlatform();
  if(p)
  {
    p->missionManager()->sendHover(m_contextMenuLocation);
    p->helmManager()->sendPilotingModeRequest("autonomous");
  }
}

void ProjectView::sendGoto()
{
  auto p = m_project->activePlatform();
  if(p)
  {
    p->missionManager()->sendGoto(m_contextMenuLocation);
    p->helmManager()->sendPilotingModeRequest("autonomous");
  }
}

void ProjectView::sendIdle()
{
  auto p = m_project->activePlatform();
  if(p)
  {
    p->missionManager()->sendIdle();
  }
}


void ProjectView::sendLookAt()
{
    //m_project->rosLink()->sendLookAt(m_contextMenuLocation);
}

void ProjectView::sendLookAtASV()
{
    //m_project->rosLink()->sendLookAtMode("follow_vehicle");
}

void ProjectView::beforeUpdateBackground()
{
    BackgroundRaster *bg =  m_project->getBackgroundRaster();
    if(bg)
    {
        QRect view = frameRect();
        QPointF center = mapToScene(view.center());
        m_savedCenter = bg->pixelToGeo(center);
    }
    else
    {
        // clear saved position
        m_savedCenter = QGeoCoordinate();
    }
    qDebug() << "saved center: " << m_savedCenter;

}

void ProjectView::updateBackground(BackgroundRaster* bg)
{
    auto bgRect = bg->boundingRect();
    setSceneRect(bgRect.marginsAdded(QMarginsF(bgRect.width()*.75,bgRect.height()*.75,bgRect.width()*.75,bgRect.height()*.75)));
    if(m_savedCenter.isValid())
    {
        QPointF center = bg->geoToPixel(m_savedCenter);
        centerOn(center);
    }
}

void ProjectView::centerMap(QGeoCoordinate location)
{
    BackgroundRaster *bg =  m_project->getBackgroundRaster();
    if(bg)
    {
        QPointF center = bg->geoToPixel(location);
        centerOn(center);    
    }  
}

void ProjectView::sendViewport()
{
    QPointF ll = mapToScene(frameRect().bottomLeft());
    QPointF ur = mapToScene(frameRect().topRight());
    emit viewportChanged(ll,ur);
}