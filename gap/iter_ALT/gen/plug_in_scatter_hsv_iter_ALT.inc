/* ----------------------------------------------------------------------
 * p_plug_in_scatter_hsv_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_scatter_hsv_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_scatter_hsv_Vals 
    {
      gint      holdness;
      gint      hue_distance;
      gint      saturation_distance;
      gint      value_distance;
      gboolean  preview;
    } t_plug_in_scatter_hsv_Vals; 

    t_plug_in_scatter_hsv_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_scatter_hsv_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_scatter_hsv_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_scatter_hsv_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_scatter_hsv_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_scatter_hsv_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_scatter_hsv_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_scatter_hsv_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gint(&buf.holdness, buf_from->holdness, buf_to->holdness, total_steps, current_step);
    p_delta_gint(&buf.hue_distance, buf_from->hue_distance, buf_to->hue_distance, total_steps, current_step);
    p_delta_gint(&buf.saturation_distance, buf_from->saturation_distance, buf_to->saturation_distance, total_steps, current_step);
    p_delta_gint(&buf.value_distance, buf_from->value_distance, buf_to->value_distance, total_steps, current_step);

    buf.preview = FALSE;
    gimp_set_data("plug_in_scatter_hsv", &buf, sizeof(buf)); 

    return 0; /* OK */
}
