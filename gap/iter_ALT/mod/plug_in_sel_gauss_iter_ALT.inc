/* ----------------------------------------------------------------------
 * p_plug_in_sel_gauss_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_sel_gauss_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_sel_gauss_Vals 
    {
      gdouble   radius;
      gint      maxdelta;
    } t_plug_in_sel_gauss_Vals; 

    t_plug_in_sel_gauss_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_sel_gauss_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_sel_gauss_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_sel_gauss_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_sel_gauss_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_sel_gauss_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_sel_gauss_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_sel_gauss_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.radius, buf_from->radius, buf_to->radius, total_steps, current_step);
    p_delta_gint(&buf.maxdelta, buf_from->maxdelta, buf_to->maxdelta, total_steps, current_step);

    gimp_set_data("plug_in_sel_gauss", &buf, sizeof(buf)); 

    return 0; /* OK */
}
