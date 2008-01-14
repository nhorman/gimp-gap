/* ----------------------------------------------------------------------
 * p_plug_in_oilify_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_oilify_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_oilify_Vals 
    {
      gdouble  mask_size;
      gboolean use_mask_size_map;
      gint     mask_size_map;
      gdouble  exponent;
      gboolean use_exponent_map;
      gint     exponent_map;
      gint     mode;               /* is used as boolean flag (therefore do not iterate) */
    } t_plug_in_oilify_Vals; 

    t_plug_in_oilify_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_oilify_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_oilify_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_oilify_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug-in-oilify-ITER-FROM", g_plugin_data_from); 
    gimp_get_data("plug-in-oilify-ITER-TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_oilify_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_oilify_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.mask_size, buf_from->mask_size, buf_to->mask_size, total_steps, current_step);
    p_delta_gdouble(&buf.exponent, buf_from->exponent, buf_to->exponent, total_steps, current_step);

    if(buf.use_mask_size_map)
    {
      p_delta_gintdrawable(&buf.mask_size_map, buf_from->mask_size_map, buf_to->mask_size_map, total_steps, current_step);
    }
    if(buf.use_exponent_map)
    {
      p_delta_gintdrawable(&buf.exponent_map, buf_from->exponent_map, buf_to->exponent_map, total_steps, current_step);
    }

    gimp_set_data("plug-in-oilify", &buf, sizeof(buf)); 

    return 0; /* OK */
}
