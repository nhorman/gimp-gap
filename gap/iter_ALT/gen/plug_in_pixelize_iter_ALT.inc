/* ----------------------------------------------------------------------
 * p_plug_in_pixelize_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_pixelize_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_pixelize_Vals 
    {
      gint pixelwidth;
      gint pixelheight;
    } t_plug_in_pixelize_Vals; 

    t_plug_in_pixelize_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_pixelize_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_pixelize_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_pixelize_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug-in-pixelize-ITER-FROM", g_plugin_data_from); 
    gimp_get_data("plug-in-pixelize-ITER-TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_pixelize_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_pixelize_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gint(&buf.pixelwidth, buf_from->pixelwidth, buf_to->pixelwidth, total_steps, current_step);
    p_delta_gint(&buf.pixelheight, buf_from->pixelheight, buf_to->pixelheight, total_steps, current_step);

    gimp_set_data("plug-in-pixelize", &buf, sizeof(buf)); 

    return 0; /* OK */
}
