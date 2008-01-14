/* ----------------------------------------------------------------------
 * p_plug_in_grid_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_grid_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_grid_Vals 
    {
      gint      hwidth;
      gint      hspace;
      gint      hoffset;
      GimpRGB   hcolor;
      gint      vwidth;
      gint      vspace;
      gint      voffset;
      GimpRGB   vcolor;
      gint      iwidth;
      gint      ispace;
      gint      ioffset;
      GimpRGB   icolor;
    } t_plug_in_grid_Vals; 

    t_plug_in_grid_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_grid_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_grid_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_grid_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug-in-grid-ITER-FROM", g_plugin_data_from); 
    gimp_get_data("plug-in-grid-ITER-TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_grid_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_grid_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gint(&buf.hwidth, buf_from->hwidth, buf_to->hwidth, total_steps, current_step);
    p_delta_gint(&buf.hspace, buf_from->hspace, buf_to->hspace, total_steps, current_step);
    p_delta_gint(&buf.hoffset, buf_from->hoffset, buf_to->hoffset, total_steps, current_step);
    p_delta_GimpRGB(&buf.hcolor, &buf_from->hcolor, &buf_to->hcolor, total_steps, current_step);
    p_delta_gint(&buf.vwidth, buf_from->vwidth, buf_to->vwidth, total_steps, current_step);
    p_delta_gint(&buf.vspace, buf_from->vspace, buf_to->vspace, total_steps, current_step);
    p_delta_gint(&buf.voffset, buf_from->voffset, buf_to->voffset, total_steps, current_step);
    p_delta_GimpRGB(&buf.vcolor, &buf_from->vcolor, &buf_to->vcolor, total_steps, current_step);
    p_delta_gint(&buf.iwidth, buf_from->iwidth, buf_to->iwidth, total_steps, current_step);
    p_delta_gint(&buf.ispace, buf_from->ispace, buf_to->ispace, total_steps, current_step);
    p_delta_gint(&buf.ioffset, buf_from->ioffset, buf_to->ioffset, total_steps, current_step);
    p_delta_GimpRGB(&buf.icolor, &buf_from->icolor, &buf_to->icolor, total_steps, current_step);
 
    gimp_set_data("plug-in-grid", &buf, sizeof(buf)); 

    return 0; /* OK */
}
