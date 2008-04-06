/* ----------------------------------------------------------------------
 * p_plug_in_exchange_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_exchange_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_exchange_Vals 
    {
      GimpRGB  from;
      GimpRGB  to;
      GimpRGB  threshold;
    } t_plug_in_exchange_Vals; 

    t_plug_in_exchange_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_exchange_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_exchange_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_exchange_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug-in-exchange-ITER-FROM", g_plugin_data_from); 
    gimp_get_data("plug-in-exchange-ITER-TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_exchange_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_exchange_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_GimpRGB(&buf.from, &buf_from->from, &buf_to->from, total_steps, current_step);
    p_delta_GimpRGB(&buf.to, &buf_from->to, &buf_to->to, total_steps, current_step);
    p_delta_GimpRGB(&buf.threshold, &buf_from->threshold, &buf_to->threshold, total_steps, current_step);

    /* exchange takes FROM colors from foreground when called in mode GIMP_RUN_WITH_LAST_VALS */
    gimp_context_set_foreground(&buf.from);
    
    gimp_set_data("plug-in-exchange", &buf, sizeof(buf)); 

    return 0; /* OK */
}
