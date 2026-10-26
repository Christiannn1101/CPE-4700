#include <gtk/gtk.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>

#include "balance_loop.h"
#include "gui.h"

extern atomic_bool running;          /* defined in maingui.c */

static GtkLabel *pitch_label, *rpm0_label, *rpm1_label, *info_label;
static GtkAdjustment *kp_adj, *ki_adj, *kd_adj;
static GMainLoop *main_loop;

/* Gains at startup (from balance_loop.c)*/
static double def_kp, def_ki, def_kd;

static void update_labels(void)
{
    char buf[96];

    snprintf(buf, sizeof(buf), "Pitch: %.2f deg", atomic_load(&g_pitch_deg));
    gtk_label_set_text(pitch_label, buf);

    snprintf(buf, sizeof(buf), "Motor 0: %.1f RPM", atomic_load(&g_motor_rpm[0]));
    gtk_label_set_text(rpm0_label, buf);

    snprintf(buf, sizeof(buf), "Motor 1: %.1f RPM", atomic_load(&g_motor_rpm[1]));
    gtk_label_set_text(rpm1_label, buf);

    snprintf(buf, sizeof(buf), "Rate: %.1f deg/s   Duty: %.2f / %.2f",
             atomic_load(&g_rpm_ref),
             atomic_load(&g_duty[0]), atomic_load(&g_duty[1]));
    gtk_label_set_text(info_label, buf);
}

static gboolean gui_timer_cb(gpointer user_data)
{
    (void)user_data;
    if (!running) {                    
        g_main_loop_quit(main_loop);
        return G_SOURCE_REMOVE;
    }
    update_labels();
    return G_SOURCE_CONTINUE;
}

static void gains_changed_cb(GtkAdjustment *adj, gpointer user_data)
{
    (void)adj; (void)user_data;
    balance_loop_set_gains(gtk_adjustment_get_value(kp_adj),
                           gtk_adjustment_get_value(ki_adj),
                           gtk_adjustment_get_value(kd_adj));
}

static void reset_gains_cb(GtkButton *button, gpointer user_data)
{
    (void)button; (void)user_data;
    gtk_adjustment_set_value(kp_adj, def_kp);
    gtk_adjustment_set_value(ki_adj, def_ki);
    gtk_adjustment_set_value(kd_adj, def_kd);
}

static void quit_cb(GtkButton *button, gpointer user_data)
{
    (void)button; (void)user_data;
    running = false;
    g_main_loop_quit(main_loop);
}

static gboolean close_cb(GtkWindow *w, gpointer user_data)
{
    (void)w; (void)user_data;
    running = false;
    g_main_loop_quit(main_loop);
    return FALSE;
}

/* One row: label + slider + spin button sharing one adjustment */
static GtkAdjustment *add_gain_row(GtkWidget *grid, int row, const char *name,
                                   double init, double max, double step, int digits)
{
    GtkAdjustment *adj = gtk_adjustment_new(init, 0.0, max, step, step * 10.0, 0.0);
    GtkWidget *label = gtk_label_new(name);
    GtkWidget *scale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, adj);
    GtkWidget *spin  = gtk_spin_button_new(adj, step, digits);

    gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
    gtk_widget_set_hexpand(scale, TRUE);

    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), scale, 1, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), spin,  2, row, 1, 1);

    g_signal_connect(adj, "value-changed", G_CALLBACK(gains_changed_cb), NULL);
    return adj;
}

void gui_init(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    main_loop = g_main_loop_new(NULL, FALSE);

    def_kp = atomic_load(&g_kp);
    def_ki = atomic_load(&g_ki);
    def_kd = atomic_load(&g_kd);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Self-Balancing Robot");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    g_signal_connect(window, "destroy", G_CALLBACK(close_cb), NULL);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_margin_start(grid, 16);
    gtk_widget_set_margin_end(grid, 16);
    gtk_widget_set_margin_top(grid, 16);
    gtk_widget_set_margin_bottom(grid, 16);
    
    gtk_container_add(GTK_CONTAINER(window), grid);

    pitch_label = GTK_LABEL(gtk_label_new("Pitch: 0.00 deg"));
    rpm0_label  = GTK_LABEL(gtk_label_new("Motor 0: 0.0 RPM"));
    rpm1_label  = GTK_LABEL(gtk_label_new("Motor 1: 0.0 RPM"));
    info_label  = GTK_LABEL(gtk_label_new(""));

    gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(pitch_label), 0, 0, 3, 1);
    gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(rpm0_label),  0, 1, 3, 1);
    gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(rpm1_label),  0, 2, 3, 1);
    gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(info_label),  0, 3, 3, 1);

    /* Ranges match your gain scale: kp ~0.05, kd ~0.0015 */
    kp_adj = add_gain_row(grid, 4, "Kp", def_kp, 0.20,  0.001,  3);
    ki_adj = add_gain_row(grid, 5, "Ki", def_ki, 0.001, 0.00001, 5);
    kd_adj = add_gain_row(grid, 6, "Kd", def_kd, 0.01,  0.0001, 4);

    GtkWidget *reset_button = gtk_button_new_with_label("Reset Gains");
    GtkWidget *quit_button  = gtk_button_new_with_label("Quit");
    gtk_grid_attach(GTK_GRID(grid), reset_button, 0, 7, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), quit_button,  2, 7, 1, 1);

    g_signal_connect(reset_button, "clicked", G_CALLBACK(reset_gains_cb), NULL);
    g_signal_connect(quit_button,  "clicked", G_CALLBACK(quit_cb), NULL);

    g_timeout_add(50, gui_timer_cb, NULL);   

    gtk_widget_show_all(window);             

    g_main_loop_run(main_loop);              
    g_main_loop_unref(main_loop);
}