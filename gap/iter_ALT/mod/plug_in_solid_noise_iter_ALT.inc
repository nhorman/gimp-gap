/* ----------------------------------------------------------------------
 * p_plug_in_solid_noise_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_solid_noise_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_solid_noise_Vals 
    {
      gboolean    tilable;
      gboolean    turbulent;
      guint       seed;
      gint        detail;
      gdouble     xsize;
      gdouble     ysize;
      gboolean    random_seed;
      gboolean    preview;
    } t_plug_in_solid_noise_Vals; 

    t_plug_in_solid_noise_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_solid_noise_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_solid_noise_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_solid_noise_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_solid_noise_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_solid_noise_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_solid_noise_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_solid_noise_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_guint(&buf.seed, buf_from->seed, buf_to->seed, total_steps, current_step);
    p_delta_gint(&buf.detail, buf_from->detail, buf_to->detail, total_steps, current_step);
    p_delta_gdouble(&buf.xsize, buf_from->xsize, buf_to->xsize, total_steps, current_step);
    p_delta_gdouble(&buf.ysize, buf_from->ysize, buf_to->ysize, total_steps, current_step);

    buf.preview = FALSE;
    gimp_set_data("plug_in_solid_noise", &buf, sizeof(buf)); 

    return 0; /* OK */
}
