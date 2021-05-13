#include "detailsview.h"
#include <QStandardItemModel>
#include <QVBoxLayout>
#include "autonomousvehicleproject.h"
#include "backgroundraster.h"
#include "backgrounddetails.h"
#include "waypoint.h"
#include "waypointdetails.h"
#include "trackline.h"
#include "tracklinedetails.h"
#include "surveypattern.h"
#include "surveypatterndetails.h"
#include "platform.h"
#include "platformdetails.h"
#include "behavior.h"
#include "behaviordetails.h"

#include <QDebug>

DetailsView::DetailsView(QWidget *parent) : QWidget(parent), m_project(nullptr),currentWidget(nullptr)
{
    backgroundDetails = new BackgroundDetails(this);
    backgroundDetails->hide();
    waypointDetails = new WaypointDetails(this);
    waypointDetails->hide();
    trackLineDetails = new TrackLineDetails(this);
    trackLineDetails->hide();
    surveyPatternDetails = new SurveyPatternDetails(this);
    surveyPatternDetails->hide();
    platformDetails = new PlatformDetails(this);
    platformDetails->hide();
    behaviorDetails = new BehaviorDetails(this);
    behaviorDetails->hide();

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(backgroundDetails);
    layout->addWidget(waypointDetails);
    layout->addWidget(trackLineDetails);
    layout->addWidget(surveyPatternDetails);
    layout->addWidget(platformDetails);
    layout->addWidget(behaviorDetails);
    setLayout(layout);
}

QSize DetailsView::sizeHint() const
{
    if(currentWidget)
        return currentWidget->sizeHint();
    return QWidget::sizeHint();
}

void DetailsView::setProject(AutonomousVehicleProject *project)
{
    m_project = project;
}

void DetailsView::setCurrentWidget(QWidget *widget)
{
    if(currentWidget != widget)
    {
        if(currentWidget)
            currentWidget->hide();
        currentWidget = widget;
        if(currentWidget)
        {
            currentWidget->show();
            updateGeometry();
        }
    }
}

void DetailsView::onCurrentItemChanged(const QModelIndex &current, const QModelIndex &previous)
{
    MissionItem* mi = m_project->itemFromIndex(current);
    qDebug() << "metaobject class name: " << mi->metaObject()->className();
    QString itemType = mi->metaObject()->className();

    if (itemType == "BackgroundRaster")
    {
        BackgroundRaster *bg = qobject_cast<BackgroundRaster*>(mi);
        setCurrentWidget(backgroundDetails);
        backgroundDetails->setBackgroundRaster(bg);
    }
    else if (itemType == "Waypoint")
    {
        Waypoint *wp = qobject_cast<Waypoint*>(mi);
        setCurrentWidget(waypointDetails);
        waypointDetails->setWaypoint(wp);
    }
    else if (itemType == "TrackLine")
    {
        TrackLine *tl = qobject_cast<TrackLine*>(mi);
        setCurrentWidget(trackLineDetails);
        trackLineDetails->setTrackLine(tl);
    }
    else if (itemType == "SurveyPattern")
    {
        SurveyPattern *sp = qobject_cast<SurveyPattern*>(mi);
        setCurrentWidget(surveyPatternDetails);
        surveyPatternDetails->setSurveyPattern(sp);
    }
    else if (itemType == "Platform")
    {
        Platform *p = qobject_cast<Platform*>(mi);
        setCurrentWidget(platformDetails);
        platformDetails->setPlatform(p);
    }
    else if (itemType == "Behavior")
    {
        Behavior *b = qobject_cast<Behavior*>(mi);
        setCurrentWidget(behaviorDetails);
        behaviorDetails->setBehavior(b);
    }
    else
        setCurrentWidget(nullptr);
    m_project->setCurrent(current);
}
