/* ----------------------------------------------------------------------
 * p_plug_in_illusion_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_illusion_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_illusion_Vals 
    {
      gint32     division;
      gboolean   type1;
      gboolean   type2;
    } t_plug_in_illusion_Vals; 

    t_plug_in_illusion_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_illusion_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_illusion_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_illusion_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug-in-illusion-ITER-FROM", g_plugin_data_from); 
    gimp_get_data("plug-in-illusion-ITER-TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_illusion_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_illusion_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gint32(&buf.division, buf_from->division, buf_to->division, total_steps, current_step);

    gimp_set_data("plug-in-illusion", &buf, sizeof(buf)); 

    return 0; /* OK */
}
