/* ----------------------------------------------------------------------
 * p_plug_in_small_tiles_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_small_tiles_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_small_tiles_Vals 
    {
      gint      number_of_tiles;
    } t_plug_in_small_tiles_Vals; 

    t_plug_in_small_tiles_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_small_tiles_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_small_tiles_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_small_tiles_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug-in-small-tiles-ITER-FROM", g_plugin_data_from); 
    gimp_get_data("plug-in-small-tiles-ITER-TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_small_tiles_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_small_tiles_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gint(&buf.number_of_tiles, buf_from->number_of_tiles, buf_to->number_of_tiles, total_steps, current_step);

    gimp_set_data("plug-in-small-tiles", &buf, sizeof(buf)); 

    return 0; /* OK */
}
