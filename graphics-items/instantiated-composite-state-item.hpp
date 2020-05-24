#ifndef INSTANTIATED_COMPOSITE_STATE_ITEM_HPP
#define INSTANTIATED_COMPOSITE_STATE_ITEM_HPP

#include <QGraphicsPixmapItem>
#include <QList>
#include <QPen>
#include "aera-graphics-item.hpp"

namespace aera_visualizer {

class AeraVisualizerScene;

class InstantiatedCompositeStateItem : public AeraGraphicsItem
{
public:
  InstantiatedCompositeStateItem(
    NewInstantiatedCompositeStateEvent* newInstantiatedCompositeStateEvent,
    ReplicodeObjects& replicodeObjects, AeraVisualizerScene* parent);

  /**
   * Get the values from the set of template values and other values in the icst or imdl.
   * \param source The source from replicodeObjects_.getSourceCode.
   * \param templateValues Set this to the list of template values. This first clears the list.
   * \param exposedValues Set this to the list of exposed values. This first clears the list.
   */
  static void getIcstOrImdlValues(
    std::string source, std::vector<std::string>& templateValues, std::vector<std::string>& exposedValues);

protected:
  void textItemLinkActivated(const QString& link) override;

private:
  typedef enum { HIDE_ICST, SHOW_ICST } ShowState;

  /**
   * Set factIcstHtml_ to the HTML source code for the fact and icst from newInstantiatedCompositeStateEvent_->object_.
   */
  void setFactIcstHtml();

  /**
   * Set boundCstHtml_ to the HTML source code for the cst from newInstantiatedCompositeStateEvent_->object_
   * with variables bound to the template parameters. Also set boundCstMembersHtml_ to just the set of
   * members from boundCstHtml_ .
   */
  void setBoundCstAndMembersHtml();

  /**
   * Make the full HTML for the textItem_ from factIcstHtml_, boundCstHtml_ and the object label.
   * \return The HTML.
   */
  QString makeHtml();

  NewInstantiatedCompositeStateEvent* newInstantiatedCompositeStateEvent_;
  ShowState showState_;
  QString factIcstHtml_;
  QString boundCstHtml_;
  QString boundCstMembersHtml_;
};

}

#endif
