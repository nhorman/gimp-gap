/* ----------------------------------------------------------------------
 * p_plug_in_glasstile_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_glasstile_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_glasstile_Vals 
    {
      gint      tilex;
      gint      tiley;
      /* interface only */
      gint     constrain;
    } t_plug_in_glasstile_Vals; 

    t_plug_in_glasstile_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_glasstile_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_glasstile_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_glasstile_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug-in-glasstile-ITER-FROM", g_plugin_data_from); 
    gimp_get_data("plug-in-glasstile-ITER-TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_glasstile_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_glasstile_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gint(&buf.tilex, buf_from->tilex, buf_to->tilex, total_steps, current_step);
    p_delta_gint(&buf.tiley, buf_from->tiley, buf_to->tiley, total_steps, current_step);

    gimp_set_data("plug-in-glasstile", &buf, sizeof(buf)); 

    return 0; /* OK */
}
