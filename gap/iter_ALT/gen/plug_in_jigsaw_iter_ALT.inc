/* ----------------------------------------------------------------------
 * p_plug_in_jigsaw_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_jigsaw_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef enum
    {
      BEZIER_1,
      BEZIER_2
    } t_jigsaw_style;


    typedef struct t_plug_in_jigsaw_Vals 
    {
      gint            x;
      gint            y;
      t_jigsaw_style  style;
      gint            blend_lines;
      gdouble         blend_amount;
      gboolean        preview;
      
    } t_plug_in_jigsaw_Vals; 

    t_plug_in_jigsaw_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_jigsaw_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_jigsaw_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_jigsaw_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_jigsaw_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_jigsaw_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_jigsaw_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_jigsaw_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gint(&buf.x, buf_from->x, buf_to->x, total_steps, current_step);
    p_delta_gint(&buf.y, buf_from->y, buf_to->y, total_steps, current_step);

    p_delta_gint(&buf.blend_lines, buf_from->blend_lines, buf_to->blend_lines, total_steps, current_step);
    p_delta_gdouble(&buf.blend_amount, buf_from->blend_amount, buf_to->blend_amount, total_steps, current_step);

    buf.preview = FALSE;
    gimp_set_data("plug_in_jigsaw", &buf, sizeof(buf)); 

    return 0; /* OK */
}
