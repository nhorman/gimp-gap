/* ----------------------------------------------------------------------
 * p_plug_in_papertile_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_papertile_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef enum
    {
      BACKGROUND_TYPE_TRANSPARENT,
      BACKGROUND_TYPE_INVERTED,
      BACKGROUND_TYPE_IMAGE,
      BACKGROUND_TYPE_FOREGROUND,
      BACKGROUND_TYPE_BACKGROUND,
      BACKGROUND_TYPE_COLOR
    } BackgroundType;

    typedef enum
    {
      FRACTIONAL_TYPE_BACKGROUND,           /* AS BACKGROUND */
      FRACTIONAL_TYPE_IGNORE,               /* NOT OPERATED  */
      FRACTIONAL_TYPE_FORCE                 /* FORCE DIVISION */
    } FractionalType;

    typedef struct t_plug_in_papertile_Vals 
    {
      gint32          tile_size;
      gint32          division_x;
      gint32          division_y;
      gdouble         move_max_rate;
      FractionalType  fractional_type;
      gboolean        centering;
      gboolean        wrap_around;
      BackgroundType  background_type;
      GimpRGB         background_color;
    } t_plug_in_papertile_Vals;

    t_plug_in_papertile_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_papertile_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_papertile_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_papertile_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_papertile_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_papertile_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_papertile_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_papertile_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gint32(&buf.tile_size, buf_from->tile_size, buf_to->tile_size, total_steps, current_step);
    p_delta_gint32(&buf.division_x, buf_from->division_x, buf_to->division_x, total_steps, current_step);
    p_delta_gint32(&buf.division_y, buf_from->division_y, buf_to->division_y, total_steps, current_step);
    p_delta_gdouble(&buf.move_max_rate, buf_from->move_max_rate, buf_to->move_max_rate, total_steps, current_step);
   
   /*
    * p_delta_long(&buf.fractional_type, buf_from->fractional_type, buf_to->fractional_type, total_steps, current_step);
    * p_delta_long(&buf.centering, buf_from->centering, buf_to->centering, total_steps, current_step);
    * p_delta_long(&buf.wrap_around, buf_from->wrap_around, buf_to->wrap_around, total_steps, current_step);
    * p_delta_long(&buf.background_type, buf_from->background_type, buf_to->background_type, total_steps, current_step);
    */
    
    p_delta_GimpRGB(&buf.background_color, &buf_from->background_color, &buf_to->background_color, total_steps, current_step);

    gimp_set_data("plug_in_papertile", &buf, sizeof(buf)); 

    return 0; /* OK */
}
