/* ----------------------------------------------------------------------
 * p_plug_in_softglow_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_softglow_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_softglow_Vals 
    {
      gdouble glow_radius;
      gdouble brightness;
      gdouble sharpness;
    } t_plug_in_softglow_Vals; 

    t_plug_in_softglow_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_softglow_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_softglow_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_softglow_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug-in-softglow-ITER-FROM", g_plugin_data_from); 
    gimp_get_data("plug-in-softglow-ITER-TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_softglow_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_softglow_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.glow_radius, buf_from->glow_radius, buf_to->glow_radius, total_steps, current_step);
    p_delta_gdouble(&buf.brightness, buf_from->brightness, buf_to->brightness, total_steps, current_step);
    p_delta_gdouble(&buf.sharpness, buf_from->sharpness, buf_to->sharpness, total_steps, current_step);

    /* buf.preview = FALSE; */
    gimp_set_data("plug-in-softglow", &buf, sizeof(buf)); 

    return 0; /* OK */
}
