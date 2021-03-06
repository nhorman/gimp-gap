/* ----------------------------------------------------------------------
 * p_plug_in_randomize_hurl_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_randomize_hurl_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_randomize_hurl_Vals 
    {
      gdouble   rndm_pct;
      gdouble   rndm_rcount;
      gboolean  randomize;
      guint     seed;
    } t_plug_in_randomize_hurl_Vals; 

    t_plug_in_randomize_hurl_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_randomize_hurl_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_randomize_hurl_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_randomize_hurl_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug-in-randomize-hurl-ITER-FROM", g_plugin_data_from); 
    gimp_get_data("plug-in-randomize-hurl-ITER-TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_randomize_hurl_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_randomize_hurl_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.rndm_pct, buf_from->rndm_pct, buf_to->rndm_pct, total_steps, current_step);
    p_delta_gdouble(&buf.rndm_rcount, buf_from->rndm_rcount, buf_to->rndm_rcount, total_steps, current_step);
    p_delta_guint(&buf.seed, buf_from->seed, buf_to->seed, total_steps, current_step);

    gimp_set_data("plug-in-randomize-hurl", &buf, sizeof(buf)); 

    return 0; /* OK */
}
