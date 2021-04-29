#import <Cocoa/Cocoa.h>
#include <QuartzCore/CAMetalLayer.h>
#include <QEvent>
#include <QWidget>

// taken from Arseny's comment explaining how to make Qt widgets metal compatible:
// https://github.com/KhronosGroup/MoltenVK/issues/78#issuecomment-369838674
extern "C" void *makeNSViewMetalCompatible(void *handle, bool useMetal)
{
  NSView *view = (NSView *)handle;
  assert([view isKindOfClass:[NSView class]]);

  if(useMetal && ![view.layer isKindOfClass:[CAMetalLayer class]])
  {
    [view setWantsLayer:YES];
    [view setLayer:[CAMetalLayer layer]];
  }
  else if(!useMetal && ![view.layer isKindOfClass:[CAOpenGLLayer class]])
  {
    /*
    CAOpenGLLayer* glLayer = [CAOpenGLLayer layer];
    [glLayer setAsynchronous:YES];
    [view setWantsLayer:YES];
    [view setLayer:glLayer];
    [view.layer setNeedsDisplayOnBoundsChange:YES];
    */
  }

  return view.layer;
}

void macosResizeCallback(void *view)
{
  NSView *nsView = (NSView *)view;
  assert([nsView isKindOfClass:[NSView class]]);

  QWidget *qWidget = QWidget::find((WId)nsView);
  if(qWidget)
  {
    // QEvent updateEvent(QEvent::UpdateRequest);
    //((QObject*)qWidget)->event(&updateEvent);
    // qWidget->update();
    qWidget->repaint();
  }
}
