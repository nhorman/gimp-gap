/* ----------------------------------------------------------------------
 * p_plug_in_lic_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_lic_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_lic_Vals 
    {
      gdouble  filtlen;
      gdouble  noisemag;
      gdouble  intsteps;
      gdouble  minv;
      gdouble  maxv;
      gint     effect_channel;
      gint     effect_operator;
      gint     effect_convolve;
      gint32   effect_image_id;
    } t_plug_in_lic_Vals; 

    t_plug_in_lic_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_lic_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_lic_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_lic_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_lic_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_lic_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_lic_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_lic_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.filtlen, buf_from->filtlen, buf_to->filtlen, total_steps, current_step);
    p_delta_gdouble(&buf.noisemag, buf_from->noisemag, buf_to->noisemag, total_steps, current_step);
    p_delta_gdouble(&buf.intsteps, buf_from->intsteps, buf_to->intsteps, total_steps, current_step);
    p_delta_gdouble(&buf.minv, buf_from->minv, buf_to->minv, total_steps, current_step);
    p_delta_gdouble(&buf.maxv, buf_from->maxv, buf_to->maxv, total_steps, current_step);
    p_delta_gint(&buf.effect_channel, buf_from->effect_channel, buf_to->effect_channel, total_steps, current_step);
    p_delta_gint(&buf.effect_operator, buf_from->effect_operator, buf_to->effect_operator, total_steps, current_step);
    p_delta_gint(&buf.effect_convolve, buf_from->effect_convolve, buf_to->effect_convolve, total_steps, current_step);

    /* dont iterate the effect_image_id */
    /* p_delta_gint32(&buf.effect_image_id, buf_from->effect_image_id, buf_to->effect_image_id, total_steps, current_step); */

    gimp_set_data("plug_in_lic", &buf, sizeof(buf)); 

    return 0; /* OK */
}
