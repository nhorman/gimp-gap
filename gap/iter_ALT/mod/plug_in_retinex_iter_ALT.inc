/* ----------------------------------------------------------------------
 * p_plug_in_retinex_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_retinex_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_retinex_Vals 
    {
      gint     scale;
      gint     nscales;
      gint     scales_mode;
      gfloat   cvar;
      gboolean preview;
    } t_plug_in_retinex_Vals; 

    t_plug_in_retinex_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_retinex_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_retinex_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_retinex_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_retinex_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_retinex_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_retinex_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_retinex_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gint(&buf.scale, buf_from->scale, buf_to->scale, total_steps, current_step);
    p_delta_gint(&buf.nscales, buf_from->nscales, buf_to->nscales, total_steps, current_step);
    p_delta_gint(&buf.scales_mode, buf_from->scales_mode, buf_to->scales_mode, total_steps, current_step);
    p_delta_gfloat(&buf.cvar, buf_from->cvar, buf_to->cvar, total_steps, current_step);

    buf.preview = FALSE;
    gimp_set_data("plug_in_retinex", &buf, sizeof(buf)); 

    return 0; /* OK */
}
