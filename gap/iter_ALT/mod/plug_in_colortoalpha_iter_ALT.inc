/* ----------------------------------------------------------------------
 * p_plug_in_colortoalpha_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_colortoalpha_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_colortoalpha_Vals 
    {
      GimpRGB   color;
    } t_plug_in_colortoalpha_Vals; 

    t_plug_in_colortoalpha_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_colortoalpha_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_colortoalpha_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_colortoalpha_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug-in-colortoalpha-ITER-FROM", g_plugin_data_from); 
    gimp_get_data("plug-in-colortoalpha-ITER-TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_colortoalpha_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_colortoalpha_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_GimpRGB(&buf.color, &buf_from->color, &buf_to->color, total_steps, current_step);

    gimp_set_data("plug-in-colortoalpha", &buf, sizeof(buf)); 

    return 0; /* OK */
}
