#include <gtk/gtk.h>
#include <cairo.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

// Defines the colors according to rgb standards
#define COLOUR(a, b) cairo_set_source_rgb(a, b.red, b.green, b.blue)

// Structures for coordinates, color codes and presets
typedef struct
{
  int x;
  int y;
} coordinate;

typedef struct
{
  double red;
  double green;
  double blue;
} colour_codes;

typedef struct
{
  int *p;
} preset_packet;

// Columns, Rows, Nodes, Colors, Height, Width, Block Size
int columns = 40;
int rows = 40;

char DRAW_NONE = 0,
     DRAW_WALL = 1,
     DRAW_STRT = 4,
     DRAW_END = 8,
     DRAW_VISIT = 32;

coordinate strt_node = {3, 3}, end_node = {17, 17};
colour_codes NONE_CLR = {1.00, 1.00, 1.00}, // white
    STRT_CLR = {0.11, 0.88, 0.00},          // green
    END_CLR = {1.00, 0.00, 0.00},           // red
    WALL_CLR = {0.00, 0.00, 0.00},
             BRDR_CLR = {0.11, 0.11, 0.11}, // grey
    VISIT_CLR = {0.37, 0.43, 0.50};         // 94,110,128

int BLK_sz = 25;
int width = 1500, height = 1555;
int btn_pressed = 0, node_flag = 1;
int terminal_node = 1, wall_node = 2;
int simulating = FALSE;
int ALGOS=1;
int visited[1600], inQueue[1600], inStack[1600];


// Declaration of functions
static gboolean on_draw_event(GtkWidget *, cairo_t *, gpointer);
static void draw_grid(cairo_t *);
static void draw_nodes(cairo_t *, int (*)[40]);
static void set_grid(gpointer);
static void set_algo(GtkMenuItem *, gpointer);
static void *run_bfs(void *);
static void *run_dfs(void *);
static void run_sim(GtkToolButton *, gpointer);
static void clear(GtkToolButton *, gpointer);

// Widgets for window, box and drawing area
GtkWidget *window;
GtkWidget *box1;
GtkWidget *darea;
GtkWidget *tool_bar;
GtkToolItem *bfsbtn;
GtkToolItem *dfsbtn;
GtkToolButton *stend_btn;
GtkToolButton *wall_button;
GtkToolItem *sim_barbtn;
GtkToolItem *clearbtn;

// This function is used to draw the entire grid board
static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
  // grid for nodes using preset packet
  int(*node_grid)[40] = (int(*)[40])(((preset_packet *)user_data)->p);

  // get the window settings and dimensions
  GtkWidget *win = gtk_widget_get_toplevel(widget);
  gtk_window_get_size(GTK_WINDOW(win), &width, &height);
  height -= 55;

  // Get the block size which is height divided by rows
  BLK_sz = height / rows;

  // Draw nodes and grid
  draw_nodes(cr, node_grid);
  draw_grid(cr);

  return FALSE;
}

// This sets the grid up with all the necessary block such as
// start node, end node, wall blocks, etc
static void set_grid(gpointer user_data)
{
  int(*node_grid)[40] = (int(*)[40])(((preset_packet *)user_data)->p);

  for (int i = 0; i < 40; i++)
    for (int j = 0; j < 40; j++)
      node_grid[i][j] = (i == 0 || j == 0 || i == 40 || j == 40) ? DRAW_WALL : DRAW_NONE;
  // These are the border points to set the color equal to wall or none

  // Set the start and end node in the grid
  node_grid[strt_node.y][strt_node.x] = DRAW_STRT;
  node_grid[end_node.y][end_node.x] = DRAW_END;
}

// A function used to draw lines for grid
static void draw_grid(cairo_t *cr)
{
  COLOUR(cr, BRDR_CLR);
  for (int i = 0; i <= columns; i++)
  {

    // Set the cursor properly
    cairo_move_to(cr, i * BLK_sz, 0);

    // Draw the line according to given dimensions
    cairo_line_to(cr, i * BLK_sz, rows * BLK_sz);
  }

  for (int i = 0; i <= rows; i++)
  {
    cairo_move_to(cr, 0, i * BLK_sz);
    cairo_line_to(cr, columns * BLK_sz, i * BLK_sz);
  }

  //cairo_stroke(cr);
}

// This function is used to draw the nodes onto the screen itself
static void draw_nodes(cairo_t *cr, int node_grid[40][40])
{
  for (int i = 1; i <= rows; i++)
  {
    for (int j = 1; j <= columns; j++)
    {
      // If the grid says to draw nothing
      if (node_grid[i][j] == DRAW_NONE)
        continue;

      // If the grid has start node, end node or none
      if (i == strt_node.y && j == strt_node.x)
        COLOUR(cr, STRT_CLR);
      else if (i == end_node.y && j == end_node.x)
        COLOUR(cr, END_CLR);
      else if (node_grid[i][j] & DRAW_WALL)
        COLOUR(cr, WALL_CLR);
      else if (node_grid[i][j] &= DRAW_VISIT)
        COLOUR(cr, VISIT_CLR);
      else
        COLOUR(cr, NONE_CLR);

      // Repaint the grid
      cairo_rectangle(cr, (j - 1) * BLK_sz, (i - 1) * BLK_sz, BLK_sz, BLK_sz);
      cairo_fill(cr);
    }
  }

  cairo_stroke(cr);
}

// Functions to color the nodes when clicked (mouse button)
static gboolean button_up(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  btn_pressed = 0;
}
static gboolean button_down(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  // Getting grid of nodes
  int(*node_grid)[40] = (int(*)[40])(((preset_packet *)user_data)->p);

  // Getting x and y coordinates of block that was clicked
  int x = ((int)event->x) / BLK_sz + 1,
      y = ((int)(event->y)) / BLK_sz + 1;

  btn_pressed = event->button;

  // Node flag 1 for start and stop button drawing
  if (node_flag == terminal_node)
  {
    // Events to set the start and end node according using
    // right and left click to set them
    // 1 for left click and 3 for right click
    if (event->button == 1 && (end_node.x != x || end_node.y != y))
    {
      strt_node.x = x;
      strt_node.y = y - 2;
      node_grid[y - 2][x] = DRAW_STRT;
    }
    else if (event->button == 3 && (strt_node.x != x || strt_node.y != y))
    {
      end_node.x = x;
      end_node.y = y - 2;
      node_grid[y - 2][x] = DRAW_END;
    }
  }
  else if (node_flag == 2)
  {
    // 2 for drawing walls and stuff
    if ((strt_node.x != x || strt_node.y != y) && (end_node.x != x || end_node.y != y))
    {
      if (event->button == 1)
        node_grid[y - 2][x] = DRAW_WALL;
      else if (event->button == 3)
        node_grid[y - 2][x] = DRAW_NONE;
    }
  }

  gtk_widget_queue_draw(widget);
  return TRUE;
}

// A function to toggle the node buttons between wall and start/end node selector
static void toogle_node(GtkToggleToolButton *self, gpointer user_data)
{
  switch (*(int *)user_data)
  {
  case 1:
  {
    // Set terminal node setting and deactivate wall nodes
    if (gtk_toggle_tool_button_get_active((GtkToggleToolButton *)stend_btn))
    {
      node_flag = terminal_node;
      gtk_toggle_tool_button_set_active((GtkToggleToolButton *)wall_button, FALSE);
    }
    break;
  }
  case 2:
  {
    // Set wall node setting and deactivate terminal nodes
    if (gtk_toggle_tool_button_get_active((GtkToggleToolButton *)wall_button))
    {
      node_flag = wall_node;
      gtk_toggle_tool_button_set_active((GtkToggleToolButton *)stend_btn, FALSE);
    }
    break;
  }
  }
}
// BFS Implementation

struct QueueNode
{
  coordinate value;
  struct QueueNode *next;
};

// Queue Functions

struct QueueNode *front = NULL;
struct QueueNode *rear = NULL;

static int EmptyQueue(void)
{
  return ((front == NULL) && (rear == NULL));
}

static void Enqueue(coordinate value)
{
  struct QueueNode *temp = (struct QueueNode *)malloc(sizeof(struct QueueNode));
  temp->value = value;
  temp->next = NULL;
  if (EmptyQueue())
  {
    rear = temp;
    front = temp;
  }
  else
  {
    rear->next = temp;
    rear = temp;
  }
}

static coordinate Dequeue(void)
{
  if (front != NULL)
  {
    coordinate temp = front->value;
    struct QueueNode *t = front;
    front = front->next;
    if (t == rear)
    {
      rear = NULL;
    }

    free(t);

    return temp;
  }
}

static void listbfs(int draw_grid[rows][columns], coordinate strt, coordinate end, GtkWidget *widget)
{
  for (int i = 0; i < 1600; i++)
  {
    visited[i] = 0;
    inQueue[i] = 0;
  }
  coordinate current = strt;
  Enqueue(current);
  while ((visited[end.x + end.y*40] != 1) && !EmptyQueue())
  {
    if (visited[current.x + current.y * 40] == 0)
    {
      if (current.x >= 0 && current.x < 40 && current.y >= 0 && current.y < 40)
      {
        if (current.x > 0)
        {
          if ((inQueue[current.x - 1 + (current.y * 40)] == 0) && draw_grid[current.y][current.x-1] != DRAW_WALL)
          {
            coordinate temp;
            temp.x = current.x - 1;
            temp.y = current.y;
            Enqueue(temp);
            inQueue[current.x - 1 + (current.y * 40)] = 1;
          }
        }
        if (current.y > 0)
        {
          if (inQueue[current.x + (current.y - 1) * 40] == 0 && draw_grid[current.y-1][current.x] != DRAW_WALL)
          {
            coordinate temp;
            temp.x = current.x;
            temp.y = current.y - 1;
            Enqueue(temp);
            inQueue[current.x + (current.y - 1) * 40] = 1;
          }
        }

        if (current.x < 39)
        {
          if (inQueue[current.x + 1 + (current.y * 40)] == 0 && draw_grid[current.y][current.x+1] != DRAW_WALL)
          {
            coordinate temp;
            temp.x = current.x + 1;
            temp.y = current.y;
            Enqueue(temp);
            inQueue[current.x + 1 + (current.y * 40)] = 1;
          }
        }

        if (current.y < 39)
        {
          if (inQueue[current.x + (current.y + 1) * 40] == 0 && draw_grid[current.y+1][current.x] != DRAW_WALL)
          {
            coordinate temp;
            temp.x = current.x;
            temp.y = current.y + 1;
            Enqueue(temp);
            inQueue[current.x + (current.y + 1) * 40] = 1;
          }
        }

        visited[current.x + current.y * 40] = 1;
        draw_grid[current.y][current.x] |= DRAW_VISIT;
        printf("\n%d, %d", current.x, current.y);
      }
    }
    current = Dequeue();
    usleep(10000);
    gtk_widget_queue_draw(window);
  }
  gtk_widget_queue_draw(widget);
}
// BFS END

// DFS Implementation

struct StackNode
{
  coordinate value;
  struct StackNode *next;
};

struct StackNode *head = NULL;

// Stack Functions

static int EmptyStack(void) {
  return (head == NULL);
}

static void Push(coordinate value) {
  struct StackNode *temp = (struct StackNode*) malloc (sizeof(struct StackNode));
  temp->value = value;
  temp->next = NULL;

  if (EmptyStack()) {
    head = temp;
  } else {
    temp->next = head;
    head = temp;
  }
}

static coordinate Pop(void) {
  if (head != NULL) {
    coordinate temp = head->value;
    struct StackNode *t = head;
    head = head->next;

    free(t);
    return temp;
  }
}

static void listdfs(int draw_grid[rows][columns], coordinate strt, coordinate end, GtkWidget *widget)
{
  for (int i = 0; i < 1600; i++)
  {
    visited[i] = 0;
    inStack[i] = 0;
  }

  coordinate current = strt;
  Push(current);
  while ((visited[end.x + end.y*40] != 1) && !EmptyStack())
  {
    if (visited[current.x + current.y * 40] == 0)
    {
      if (current.x >= 0 && current.x < 40 && current.y >= 0 && current.y < 40)
      {
        if (current.x > 0)
        {
          if ((inStack[current.x - 1 + (current.y * 40)] == 0) && draw_grid[current.y][current.x-1] != DRAW_WALL)
          {
            coordinate temp;
            temp.x = current.x - 1;
            temp.y = current.y;
            Push(temp);
            inStack[current.x - 1 + (current.y * 40)] = 1;
          }
        }
        if (current.y > 0)
        {
          if (inStack[current.x + (current.y - 1) * 40] == 0 && draw_grid[current.y-1][current.x] != DRAW_WALL)
          {
            coordinate temp;
            temp.x = current.x;
            temp.y = current.y - 1;
            Push(temp);
            inStack[current.x + (current.y - 1) * 40] = 1;
          }
        }

        if (current.x < 39)
        {
          if (inStack[current.x + 1 + (current.y * 40)] == 0 && draw_grid[current.y][current.x+1] != DRAW_WALL)
          {
            coordinate temp;
            temp.x = current.x + 1;
            temp.y = current.y;
            Push(temp);
            inStack[current.x + 1 + (current.y * 40)] = 1;
          }
        }

        if (current.y < 39)
        {
          if (inStack[current.x + (current.y + 1) * 40] == 0 && draw_grid[current.y+1][current.x] != DRAW_WALL)
          {
            coordinate temp;
            temp.x = current.x;
            temp.y = current.y + 1;
            Push(temp);
            inStack[current.x + (current.y + 1) * 40] = 1;
          }
        }

        visited[current.x + current.y * 40] = 1;
        draw_grid[current.y][current.x] |= DRAW_VISIT;
        printf("\n%d, %d", current.x, current.y);
      }
    }
    current = Pop();
    usleep(10000);
    gtk_widget_queue_draw(window);
  }
  gtk_widget_queue_draw(widget);
}


// DFS END

static void *run_bfs(gpointer user_data)
{
  int(*node_grid)[40] = (int(*)[40])(((preset_packet *)user_data)->p);
  listbfs(node_grid, strt_node, end_node, window);
  simulating = FALSE;
}

static void *run_dfs(gpointer user_data) {
  int(*node_grid)[40] = (int(*)[40])(((preset_packet *)user_data)->p);
  listdfs(node_grid, strt_node, end_node, window);
  simulating = FALSE;
}

static void clearscreen(GtkToolButton *self, gpointer user_data, GtkWidget *widget) {
  int(*node_grid)[40] = (int(*)[40])(((preset_packet *)user_data)->p);

  for (int i=1; i<rows+2; i++) {
    for (int j=1; j<rows+2; j++) {
      node_grid[i][j] = DRAW_NONE;
      gtk_widget_queue_draw(widget);
    }
  }

  strt_node.x = 3;
  strt_node.y = 3;
  end_node.x = 17;
  end_node.y = 17;

  node_grid[strt_node.y][strt_node.x] = DRAW_STRT;
  node_grid[end_node.y][end_node.x] = DRAW_END;

  for (int i = 0; i < rows + 1; i++)
    node_grid[i][0] = node_grid[i][columns + 1] = DRAW_WALL;
  for (int i = 0; i < columns + 1; i++)
    node_grid[0][i] = node_grid[rows + 1][i] = DRAW_WALL;

  while(!EmptyQueue()) {
    Dequeue();
  }

  while(!EmptyStack()) {
    Pop();
  }

}

static void bfsalgo(void) {
  ALGOS=1;
}

static void dfsalgo(void) {
  ALGOS=2;
}

static void run_sim(GtkToolButton *self, gpointer user_data)
{
  if (simulating)
    return;
  simulating = TRUE;
  int(*node_grid)[40] = (int(*)[40])(((preset_packet *)user_data)->p);
  
  for (int i = 0; i < rows + 1; i++)
    node_grid[i][0] = node_grid[i][columns + 1] = DRAW_WALL;
  for (int i = 0; i < columns + 1; i++)
    node_grid[0][i] = node_grid[rows + 1][i] = DRAW_WALL;

  for (int i = 1; i < rows + 2; i++)
  {
    for (int j = 1; j < columns + 2; j++)
    {
      if (node_grid[i][j] & DRAW_WALL)
        node_grid[i][j] = DRAW_WALL;
      else if (node_grid[i][j] & DRAW_STRT)
        node_grid[i][j] = DRAW_STRT;
      else if (node_grid[i][j] & DRAW_END)
        node_grid[i][j] = DRAW_END;
      else if (node_grid[i][j] & DRAW_VISIT)
        node_grid[i][j] = DRAW_VISIT;
      else
        node_grid[i][j] = DRAW_NONE;
    }
  }

  pthread_t thread_id1;
  if (ALGOS == 1)
    pthread_create(&thread_id1, NULL, run_bfs, user_data);
  else
    pthread_create(&thread_id1, NULL, run_dfs, user_data);

}

int main(int argc, char *argv[])
{
  preset_packet presets;
  presets.p = malloc(sizeof(int) * 40 * 40);

  gtk_init(&argc, &argv);

  // Set the grid up with nodes
  set_grid(&presets);
  srand(time(0));

  // Making a window widget and setting it up with position, size and label
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size(GTK_WINDOW(window), width, height);
  gtk_window_set_title(GTK_WINDOW(window), "Visualizer");

  // Make a box, drawing area and add them to the window
  box1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(window), box1);

  darea = gtk_drawing_area_new();

  // A tool bar told hold everything
  tool_bar = gtk_toolbar_new();
  gtk_toolbar_set_style(GTK_TOOLBAR(tool_bar), GTK_TOOLBAR_BOTH);

  // Buttons to toggle wall mode or start/end mode or algos

  bfsbtn = gtk_tool_button_new(NULL, "BFS");
  gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), bfsbtn, -1);

  dfsbtn = gtk_tool_button_new(NULL, "DFS");
  gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), dfsbtn, -1);

  stend_btn = (GtkToolButton *)gtk_toggle_tool_button_new();
  gtk_tool_button_set_label(stend_btn, "Start/End");
  gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), (GtkToolItem *)stend_btn, -1);

  wall_button = (GtkToolButton *)gtk_toggle_tool_button_new();
  gtk_tool_button_set_label(wall_button, "Wall");
  gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), (GtkToolItem *)wall_button, -1);

  sim_barbtn = gtk_tool_button_new(NULL, "Visualize");
  gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), sim_barbtn, -1);

  clearbtn = gtk_tool_button_new(NULL, "Clear");
  gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), clearbtn, -1);

  gtk_box_pack_start(GTK_BOX(box1), tool_bar, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box1), darea, TRUE, TRUE, 0);

  rows = 40;
  columns = 40;
  // Drawing and exiting function for drawing board and window
  g_signal_connect(G_OBJECT(darea), "draw", G_CALLBACK(on_draw_event), &presets);
  g_signal_connect(window, "button-press-event", G_CALLBACK(button_down), &presets);
  g_signal_connect(window, "button-release-event", G_CALLBACK(button_up), NULL);
  g_signal_connect(G_OBJECT(bfsbtn), "clicked", G_CALLBACK(bfsalgo), &presets);
  g_signal_connect(G_OBJECT(dfsbtn), "clicked", G_CALLBACK(dfsalgo), &presets);
  g_signal_connect(G_OBJECT(stend_btn), "toggled", G_CALLBACK(toogle_node), &terminal_node);
  g_signal_connect(G_OBJECT(wall_button), "toggled", G_CALLBACK(toogle_node), &wall_node);
  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(sim_barbtn), "clicked", G_CALLBACK(run_sim), &presets);
  g_signal_connect(G_OBJECT(clearbtn), "clicked", G_CALLBACK(clearscreen), &presets);

  rows = 40;
  columns = 40;
  set_grid(&presets);

  gtk_widget_show_all(window);
  gtk_main();

  return 0;
}