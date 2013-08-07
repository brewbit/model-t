
#include "scatter_plot.h"
#include "gfx.h"


typedef struct {
  widget_t* widget;
} scatter_plot_t;


static void
scatter_plot_paint(paint_event_t* event);

static void
scatter_plot_destroy(widget_t* w);


static widget_class_t scatter_plot_widget_class = {
    .on_paint   = scatter_plot_paint,
    .on_destroy = scatter_plot_destroy
};


widget_t*
scatter_plot_create(widget_t* parent, rect_t rect)
{
  scatter_plot_t* s = calloc(1, sizeof(scatter_plot_t));
  s->widget = widget_create(parent, &scatter_plot_widget_class, s, rect);

  return s->widget;
}

static void
scatter_plot_destroy(widget_t* w)
{
  scatter_plot_t* s = widget_get_instance_data(w);
  free(s);
}

static void
scatter_plot_paint(paint_event_t* event)
{
  rect_t rect = widget_get_rect(event->widget);

  gfx_set_fg_color(WHITE);
  gfx_draw_rect(rect);
}
