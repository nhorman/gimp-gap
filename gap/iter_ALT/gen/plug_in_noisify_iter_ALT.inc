/* ----------------------------------------------------------------------
 * p_plug_in_noisify_iter_ALT 
 * ----------------------------------------------------------------------
 * this plug-in registers 2 procedures named
 *   plug_in_scatter_rgb  and
 *   plug_in_noisify
 *
 * the Last values Buffer is always stored with the name "plug_in_scatter_rgb"
 * therefore the iterator must use the plug_in_scatter_rgb based names.
 * (there is no iterator support possible for the other API named plug_in_noisify)
 */
gint p_plug_in_noisify_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_noisify_Vals 
    {
      gboolean  independent;
      gboolean  correlated ;
      gdouble   noise[4];
    } t_plug_in_noisify_Vals;
    gint ii;

    t_plug_in_noisify_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_noisify_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_noisify_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_noisify_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_scatter_rgb_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_scatter_rgb_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_noisify_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_noisify_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    for(ii=0;ii<4;ii++)
    {
      p_delta_gdouble(&buf.noise[ii], buf_from->noise[ii], buf_to->noise[ii], total_steps, current_step);
    }

    gimp_set_data("plug_in_scatter_rgb", &buf, sizeof(buf)); 

    return 0; /* OK */
}
