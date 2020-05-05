#include <fstream>
#include "arrow.hpp"
#include "aera-model-item.hpp"
#include "aera-visualizer-scene.hpp"
#include "aera-visualizer-window.hpp"

#include <QtWidgets>

using namespace std;
using namespace std::chrono;
using namespace core;
using namespace r_code;

namespace aera_visualizer {

AeraVisulizerWindow::AeraVisulizerWindow()
: AeraVisulizerWindowBase(0),
  iNextEvent_(0)
{
  createActions();
  createMenus();

  string userOperatorsFilePath = "C:\\Users\\Jeff\\AERA\\replicode\\Test\\V1.2\\user.classes.replicode";
  string decompiledFilePath = "C:\\Users\\Jeff\\AERA\\replicode\\Test\\decompiled_objects.txt";
  string consoleOutputFilePath = "C:\\Users\\Jeff\\temp\\Test.out.txt";

  string error = replicodeObjects_.init(userOperatorsFilePath, decompiledFilePath);
  setTimeReference(replicodeObjects_.getTimeReference());

  scene_ = new AeraVisualizerScene(itemMenu_, replicodeObjects_, this);
  scene_->setSceneRect(QRectF(0, 0, 5000, 5000));
  connect(scene_, SIGNAL(itemInserted(AeraModelItem*)),
    this, SLOT(itemInserted(AeraModelItem*)));
  createToolbars();

  QVBoxLayout* centralLayout = new QVBoxLayout();
  QGraphicsView* view = new QGraphicsView(scene_, this);
  view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  centralLayout->addWidget(view);
  centralLayout->addWidget(getPlayerControlPanel());

  QWidget* centralWidget = new QWidget();
  centralWidget->setLayout(centralLayout);
  setCentralWidget(centralWidget);

  setWindowTitle(tr("AERA Visualizer"));
  setUnifiedTitleAndToolBarOnMac(true);

  addEvents(consoleOutputFilePath);
}

void AeraVisulizerWindow::addEvents(const string& consoleOutputFilePath)
{
  ifstream consoleOutputFile(consoleOutputFilePath);
  regex newModelRegex("^(\\d+)s:(\\d+)ms:(\\d+)us -> mdl (\\d+)$");
  regex setEvidenceCountAndSuccessRateRegex("^(\\d+)s:(\\d+)ms:(\\d+)us mdl (\\d+) cnt:(\\d+) sr:([\\d\\.]+)$");

  string line;
  while (getline(consoleOutputFile, line)) {
    smatch matches;

    if (regex_search(line, matches, newModelRegex))
      // Assume the initial success rate is 1.
      events_.push_back(make_shared<NewModelEvent>(
        getTimestamp(matches), replicodeObjects_.getObject(stol(matches[4].str())), 1, 1));
    else if (regex_search(line, matches, setEvidenceCountAndSuccessRateRegex))
      // Assume the initial success rate is 1.
      events_.push_back(make_shared<SetModelEvidenceCountAndSuccessRateEvent>(
        getTimestamp(matches), replicodeObjects_.getObject(stol(matches[4].str())), stol(matches[5].str()),
        stof(matches[6].str())));
  }
}

Timestamp AeraVisulizerWindow::getTimestamp(const smatch& matches)
{
  microseconds us(1000000 * stoll(matches[1].str()) +
                     1000 * stoll(matches[2].str()) +
                            stoll(matches[3].str()));
  return replicodeObjects_.getTimeReference() + us;
}

Timestamp AeraVisulizerWindow::stepEvent(Timestamp maximumTime)
{
  if (iNextEvent_ >= events_.size())
    // Return the value meaning no change.
    return Utils_MaxTime;

  AeraEvent* event = events_[iNextEvent_].get();
  if (event->time_ > maximumTime)
    return Utils_MaxTime;

  if (event->eventType_ == NewModelEvent::EVENT_TYPE) {
    auto newModelEvent = (NewModelEvent*)event;

    // Restore the evidence count and success rate in case we did a rewind.
    newModelEvent->model_->code(MDL_CNT) = Atom::Float(newModelEvent->evidenceCount_);
    newModelEvent->model_->code(MDL_SR) = Atom::Float(newModelEvent->successRate_);

    // Add the new model.
    AeraModelItem* newItem = scene_->addAeraModelItem(newModelEvent);

    // Add arrows to all referenced models.
    for (int i = 0; i < newModelEvent->model_->references_size(); ++i) {
      // TODO: Make this work for objects other than models.
      auto referencedObject = scene_->getAeraModelItem(
        newModelEvent->model_->get_reference(i)->get_oid());
      if (referencedObject)
        scene_->addArrow(newItem, referencedObject);
    }

    scene_->establishFlashTimer();
  }
  else if (event->eventType_ == SetModelEvidenceCountAndSuccessRateEvent::EVENT_TYPE) {
    auto setSuccessRateEvent = (SetModelEvidenceCountAndSuccessRateEvent*)event;

    // Save the current values for a later undo.
    setSuccessRateEvent->oldEvidenceCount_ = setSuccessRateEvent->model_->code(MDL_CNT).asFloat();
    setSuccessRateEvent->oldSuccessRate_ = setSuccessRateEvent->model_->code(MDL_SR).asFloat();

    // Update the model.
    setSuccessRateEvent->model_->code(MDL_CNT) = Atom::Float(setSuccessRateEvent->evidenceCount_);
    setSuccessRateEvent->model_->code(MDL_SR) = Atom::Float(setSuccessRateEvent->successRate_);

    auto modelItem = scene_->getAeraModelItem(setSuccessRateEvent->model_->get_oid());
    if (modelItem) {
      modelItem->updateFromModel();
      if (setSuccessRateEvent->evidenceCount_ != setSuccessRateEvent->oldEvidenceCount_ &&
          setSuccessRateEvent->successRate_ == setSuccessRateEvent->oldSuccessRate_)
        // Only the evidence count changed.
        modelItem->evidenceCountFlashCountdown_ = 6;
      else if (setSuccessRateEvent->evidenceCount_ == setSuccessRateEvent->oldEvidenceCount_ &&
        setSuccessRateEvent->successRate_ != setSuccessRateEvent->oldSuccessRate_)
        // Only the success rate changed.
        modelItem->successRateFlashCountdown_ = 6;
      else {
        modelItem->evidenceCountFlashCountdown_ = 6;
        modelItem->successRateFlashCountdown_ = 6;
      }
      scene_->establishFlashTimer();
    }
  }

  ++iNextEvent_;

  return event->time_;
}

Timestamp AeraVisulizerWindow::unstepEvent()
{
  if (iNextEvent_ == 0)
    // Return the value meaning no change.
    return Utils_MaxTime;

  --iNextEvent_;

  AeraEvent* event = events_[iNextEvent_].get();
  if (event->eventType_ == NewModelEvent::EVENT_TYPE) {
    // Find the AeraModelItem for this event and remove it.
    // Note that the event saves the updated item position and will use it when recreating the item.
    auto modelItem = scene_->getAeraModelItem(((NewModelEvent*)event)->model_->get_oid());
    if (modelItem) {
      modelItem->removeArrows();
      scene_->removeItem(modelItem);
      delete modelItem;
    }
  }
  else if (event->eventType_ == SetModelEvidenceCountAndSuccessRateEvent::EVENT_TYPE) {
    // Find the AeraModelItem for this event and set to the old evidence count and success rate.
    auto setSuccessRateEvent = (SetModelEvidenceCountAndSuccessRateEvent*)event;

    setSuccessRateEvent->model_->code(MDL_CNT) = Atom::Float(setSuccessRateEvent->oldEvidenceCount_);
    setSuccessRateEvent->model_->code(MDL_SR) = Atom::Float(setSuccessRateEvent->oldSuccessRate_);

    auto modelItem = scene_->getAeraModelItem(setSuccessRateEvent->model_->get_oid());
    if (modelItem) {
      if (setSuccessRateEvent->evidenceCount_ != setSuccessRateEvent->oldEvidenceCount_ &&
          setSuccessRateEvent->successRate_ == setSuccessRateEvent->oldSuccessRate_)
        // Only the evidence count changed.
        modelItem->evidenceCountFlashCountdown_ = 6;
      else if (setSuccessRateEvent->evidenceCount_ == setSuccessRateEvent->oldEvidenceCount_ &&
               setSuccessRateEvent->successRate_ != setSuccessRateEvent->oldSuccessRate_)
        // Only the success rate changed.
        modelItem->successRateFlashCountdown_ = 6;
      else {
        modelItem->evidenceCountFlashCountdown_ = 6;
        modelItem->successRateFlashCountdown_ = 6;
      }

      modelItem->updateFromModel();
      scene_->establishFlashTimer();
    }
  }

  if (iNextEvent_ > 0)
    return events_[iNextEvent_ - 1]->time_;
  else
    // The caller will use the time reference.
    return Timestamp(seconds(0));
}

void AeraVisulizerWindow::zoomIn()
{
  scene_->scaleViewBy(1.09);
}

void AeraVisulizerWindow::zoomOut()
{
  scene_->scaleViewBy(1 / 1.09);
}

void AeraVisulizerWindow::zoomHome()
{
  scene_->zoomViewHome();
}

void AeraVisulizerWindow::bringToFront()
{
  if (scene_->selectedItems().isEmpty())
    return;

  QGraphicsItem* selectedItem = scene_->selectedItems().first();
  QList<QGraphicsItem*> overlapItems = selectedItem->collidingItems();

  qreal zValue = 0;
  foreach(QGraphicsItem* item, overlapItems) {
    if (item->zValue() >= zValue && item->type() == AeraModelItem::Type)
      zValue = item->zValue() + 0.1;
  }
  selectedItem->setZValue(zValue);
}

void AeraVisulizerWindow::sendToBack()
{
  if (scene_->selectedItems().isEmpty())
    return;

  QGraphicsItem* selectedItem = scene_->selectedItems().first();
  QList<QGraphicsItem*> overlapItems = selectedItem->collidingItems();

  qreal zValue = 0;
  foreach(QGraphicsItem* item, overlapItems) {
    if (item->zValue() <= zValue && item->type() == AeraModelItem::Type)
      zValue = item->zValue() - 0.1;
  }
  selectedItem->setZValue(zValue);
}

void AeraVisulizerWindow::createActions()
{
  exitAction_ = new QAction(tr("E&xit"), this);
  exitAction_->setShortcuts(QKeySequence::Quit);
  exitAction_->setStatusTip(tr("Exit"));
  connect(exitAction_, SIGNAL(triggered()), this, SLOT(close()));

  zoomInAction_ = new QAction(QIcon(":/images/zoom-in.png"), tr("Zoom In"), this);
  zoomInAction_->setStatusTip(tr("Zoom In"));
  connect(zoomInAction_, SIGNAL(triggered()), this, SLOT(zoomIn()));

  zoomOutAction_ = new QAction(QIcon(":/images/zoom-out.png"), tr("Zoom Out"), this);
  zoomOutAction_->setStatusTip(tr("Zoom Out"));
  connect(zoomOutAction_, SIGNAL(triggered()), this, SLOT(zoomOut()));

  zoomHomeAction_ = new QAction(QIcon(":/images/zoom-home.png"), tr("Zoom Home"), this);
  zoomHomeAction_->setStatusTip(tr("Zoom to show all"));
  connect(zoomHomeAction_, SIGNAL(triggered()), this, SLOT(zoomHome()));

  toFrontAction_ = new QAction(tr("Bring to &Front"), this);
  toFrontAction_->setStatusTip(tr("Bring item to front"));
  connect(toFrontAction_, SIGNAL(triggered()), this, SLOT(bringToFront()));

  sendBackAction_ = new QAction(tr("Send to &Back"), this);
  sendBackAction_->setStatusTip(tr("Send item to back"));
  connect(sendBackAction_, SIGNAL(triggered()), this, SLOT(sendToBack()));
}

void AeraVisulizerWindow::createMenus()
{
  QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(exitAction_);

  QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
  viewMenu->addAction(zoomInAction_);
  viewMenu->addAction(zoomOutAction_);
  viewMenu->addAction(zoomHomeAction_);

  itemMenu_ = menuBar()->addMenu(tr("&Item"));
  itemMenu_->addAction(toFrontAction_);
  itemMenu_->addAction(sendBackAction_);
}

void AeraVisulizerWindow::createToolbars()
{
  QToolBar* toolbar = addToolBar(tr("Main"));
  toolbar->addAction(zoomInAction_);
  toolbar->addAction(zoomOutAction_);
  toolbar->addAction(zoomHomeAction_);
}

}
