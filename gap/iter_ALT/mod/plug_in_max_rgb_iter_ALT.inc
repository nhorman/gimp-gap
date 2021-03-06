/* ----------------------------------------------------------------------
 * p_plug_in_max_rgb_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_max_rgb_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_max_rgb_Vals 
    {
      gint      max_p;
    } t_plug_in_max_rgb_Vals; 

    t_plug_in_max_rgb_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_max_rgb_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_max_rgb_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_max_rgb_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug-in-max-rgb-ITER-FROM", g_plugin_data_from); 
    gimp_get_data("plug-in-max-rgb-ITER-TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_max_rgb_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_max_rgb_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gint(&buf.max_p, buf_from->max_p, buf_to->max_p, total_steps, current_step);

    gimp_set_data("plug-in-max-rgb", &buf, sizeof(buf)); 

    return 0; /* OK */
}
