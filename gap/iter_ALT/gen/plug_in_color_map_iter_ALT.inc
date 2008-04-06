/* ----------------------------------------------------------------------
 * p_plug_in_color_map_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_color_map_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    gint ii;
    typedef struct t_plug_in_color_map_Vals 
    {
      GimpRGB   colors[4];
      gint32    map_mode;
      gboolean  preview;
    } t_plug_in_color_map_Vals; 

    t_plug_in_color_map_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_color_map_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_color_map_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_color_map_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug-in-color-map-ITER-FROM", g_plugin_data_from); 
    gimp_get_data("plug-in-color-map-ITER-TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_color_map_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_color_map_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    for(ii=0; ii < 4; ii++)
    {
      p_delta_GimpRGB(&buf.colors[ii], &buf_from->colors[ii], &buf_to->colors[ii], total_steps, current_step);
    }
    p_delta_gint32(&buf.map_mode, buf_from->map_mode, buf_to->map_mode, total_steps, current_step);


    buf.preview = FALSE;

    gimp_set_data("plug-in-color-map", &buf, sizeof(buf)); 

    return 0; /* OK */
}
