/* ----------------------------------------------------------------------
 * p_plug_in_sample_colorize_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_sample_colorize_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_sample_colorize_Vals 
    {
      gint32    dst_id;             /* do not iterate this one !! */
      gint32    sample_drawable;
      gint32    hold_inten;
      gint32    orig_inten;
      gint32    rnd_subcolors;
      gint32    guess_missing;
      gint32    in_low;
      gint32    in_high;
      float     gamma;
      gint32    out_low;
      gint32    out_high;
      float     tol_col_err;
    } t_plug_in_sample_colorize_Vals; 

    t_plug_in_sample_colorize_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_sample_colorize_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_sample_colorize_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_sample_colorize_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_sample_colorize_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_sample_colorize_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_sample_colorize_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_sample_colorize_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_drawable(&buf.sample_drawable, buf_from->sample_drawable, buf_to->sample_drawable, total_steps, current_step);
    p_delta_gint32(&buf.hold_inten, buf_from->hold_inten, buf_to->hold_inten, total_steps, current_step);
    p_delta_gint32(&buf.orig_inten, buf_from->orig_inten, buf_to->orig_inten, total_steps, current_step);
    p_delta_gint32(&buf.rnd_subcolors, buf_from->rnd_subcolors, buf_to->rnd_subcolors, total_steps, current_step);
    p_delta_gint32(&buf.guess_missing, buf_from->guess_missing, buf_to->guess_missing, total_steps, current_step);
    p_delta_gint32(&buf.in_low, buf_from->in_low, buf_to->in_low, total_steps, current_step);
    p_delta_gint32(&buf.in_high, buf_from->in_high, buf_to->in_high, total_steps, current_step);
    p_delta_float(&buf.gamma, buf_from->gamma, buf_to->gamma, total_steps, current_step);
    p_delta_gint32(&buf.out_low, buf_from->out_low, buf_to->out_low, total_steps, current_step);
    p_delta_gint32(&buf.out_high, buf_from->out_high, buf_to->out_high, total_steps, current_step);
 
    p_delta_float(&buf.tol_col_err, buf_from->tol_col_err, buf_to->tol_col_err, total_steps, current_step);

    gimp_set_data("plug_in_sample_colorize", &buf, sizeof(buf)); 

    return 0; /* OK */
}
