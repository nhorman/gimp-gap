/* ----------------------------------------------------------------------
 * p_plug_in_gimpressionist_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint 
p_plug_in_gimpressionist_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
#define MAXORIENTVECT   50
#define MAXSIZEVECT     50

    typedef struct 
    {
	int                       orient_num;
	double                    orient_first;
	double                    orient_last;
	int                       orient_type;
	double                    brush_relief;
	double                    brush_scale;
	double                    brush_density;
	double                    brushgamma;
	int                       general_background_type;
	double                    general_dark_edge;
	double                    paper_relief;
	double                    paper_scale;
	int                       paper_invert;
	int                       run;
	char                      selected_brush[200];
	char                      selected_paper[200];
	GimpRGB                   color;
	
	int                       general_paint_edges;
	int                       place_type;
	t_gimpressionist_vector   orient_vectors[MAXORIENTVECT];
	int                       num_orient_vectors;
	int                       placement_center;
	double                    brush_aspect;
	double                    orient_angle_offset;
	double                    orient_strength_exponent;
	int                       general_tileable;
	int                       paper_overlay;
	int                       orient_voronoi;
	int                       color_brushes;
	int                       general_drop_shadow;
	double                    general_shadow_darkness;
	int                       size_num;
	double                    size_first;
	double                    size_last;
	int                       size_type;
	double                    devthresh;

	t_gimpressionist_smvector size_vectors[MAXSIZEVECT];
	int                       num_size_vectors;
	double                    size_strength_exponent;
	int                       size_voronoi;

	int                       general_shadow_depth;
	int                       general_shadow_blur;

	int                       color_type;
	double                    color_noise;

    } t_plug_in_gimpressionist_Vals; 





    t_plug_in_gimpressionist_Vals  buf, buf_from, buf_to;
    int                        l_idx;

    if(len_struct != sizeof(t_plug_in_gimpressionist_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_gimpressionist_iter  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_gimpressionist_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_gimpressionist_ITER_FROM", &buf_from); 
    gimp_get_data("plug_in_gimpressionist_ITER_TO",   &buf_to); 
    memcpy(&buf, &buf_from, sizeof(buf));

    p_delta_int(&buf.orient_num, buf_from.orient_num, buf_to.orient_num, total_steps, current_step);
    p_delta_gdouble(&buf.orient_first, buf_from.orient_first, buf_to.orient_first, total_steps, current_step);
    p_delta_gdouble(&buf.orient_last, buf_from.orient_last, buf_to.orient_last, total_steps, current_step);

    p_delta_int(&buf.orient_type, buf_from.orient_type, buf_to.orient_type, total_steps, current_step);
    p_delta_gdouble(&buf.brush_relief, buf_from.brush_relief, buf_to.brush_relief, total_steps, current_step);
    p_delta_gdouble(&buf.brush_scale, buf_from.brush_scale, buf_to.brush_scale, total_steps, current_step);
    p_delta_gdouble(&buf.brush_density, buf_from.brush_density, buf_to.brush_density, total_steps, current_step);
    p_delta_gdouble(&buf.brushgamma, buf_from.brushgamma, buf_to.brushgamma, total_steps, current_step);

    p_delta_int(&buf.general_background_type, buf_from.general_background_type, buf_to.general_background_type, total_steps, current_step);
    p_delta_gdouble(&buf.general_dark_edge, buf_from.general_dark_edge, buf_to.general_dark_edge, total_steps, current_step);
    p_delta_gdouble(&buf.paper_relief, buf_from.paper_relief, buf_to.paper_relief, total_steps, current_step);
    p_delta_gdouble(&buf.paper_scale, buf_from.paper_scale, buf_to.paper_scale, total_steps, current_step);

    p_delta_int(&buf.paper_invert, buf_from.paper_invert, buf_to.paper_invert, total_steps, current_step);


/*  p_delta_int(&buf.run, buf_from.run, buf_to.run, total_steps, current_step); */

    p_delta_GimpRGB(&buf.color, &buf_from.color, &buf_to.color, total_steps, current_step);


    p_delta_int(&buf.general_paint_edges, buf_from.general_paint_edges, buf_to.general_paint_edges, total_steps, current_step);
    p_delta_int(&buf.place_type, buf_from.place_type, buf_to.place_type, total_steps, current_step);

    for(l_idx = 0; l_idx < MAXORIENTVECT; l_idx++)
    {
      p_delta_gimpressionist_vector(&buf.orient_vectors[l_idx], &buf_from.orient_vectors[l_idx], &buf_to.orient_vectors[l_idx], total_steps, current_step);
    }



    p_delta_int(&buf.num_orient_vectors, buf_from.num_orient_vectors, buf_to.num_orient_vectors, total_steps, current_step);
    p_delta_int(&buf.placement_center, buf_from.placement_center, buf_to.placement_center, total_steps, current_step);
    p_delta_gdouble(&buf.brush_aspect, buf_from.brush_aspect, buf_to.brush_aspect, total_steps, current_step);
    p_delta_gdouble(&buf.orient_angle_offset, buf_from.orient_angle_offset, buf_to.orient_angle_offset, total_steps, current_step);
    p_delta_gdouble(&buf.orient_strength_exponent, buf_from.orient_strength_exponent, buf_to.orient_strength_exponent, total_steps, current_step);

    /* p_delta_int(&buf.general_tileable, buf_from.general_tileable, buf_to.general_tileable, total_steps, current_step); */
    p_delta_int(&buf.paper_overlay, buf_from.paper_overlay, buf_to.paper_overlay, total_steps, current_step);
    p_delta_int(&buf.orient_voronoi, buf_from.orient_voronoi, buf_to.orient_voronoi, total_steps, current_step);
    p_delta_int(&buf.color_brushes, buf_from.color_brushes, buf_to.color_brushes, total_steps, current_step);
    /* p_delta_int(&buf.general_drop_shadow, buf_from.general_drop_shadow, buf_to.general_drop_shadow, total_steps, current_step); */


    p_delta_gdouble(&buf.general_shadow_darkness, buf_from.general_shadow_darkness, buf_to.general_shadow_darkness, total_steps, current_step);
    p_delta_int(&buf.size_num, buf_from.size_num, buf_to.size_num, total_steps, current_step);
    p_delta_gdouble(&buf.size_first, buf_from.size_first, buf_to.size_first, total_steps, current_step);
    p_delta_gdouble(&buf.size_last, buf_from.size_last, buf_to.size_last, total_steps, current_step);
    p_delta_int(&buf.size_type, buf_from.size_type, buf_to.size_type, total_steps, current_step);
    p_delta_gdouble(&buf.devthresh, buf_from.devthresh, buf_to.devthresh, total_steps, current_step);
 
    for(l_idx = 0; l_idx < MAXSIZEVECT; l_idx++)
    {
      p_delta_gimpressionist_smvector(&buf.size_vectors[l_idx], &buf_from.size_vectors[l_idx], &buf_to.size_vectors[l_idx], total_steps, current_step);
    }
    

    p_delta_int(&buf.num_size_vectors, buf_from.num_size_vectors, buf_to.num_size_vectors, total_steps, current_step);
    p_delta_gdouble(&buf.size_strength_exponent, buf_from.size_strength_exponent, buf_to.size_strength_exponent, total_steps, current_step);
    p_delta_int(&buf.size_voronoi, buf_from.size_voronoi, buf_to.size_voronoi, total_steps, current_step);

    p_delta_int(&buf.general_shadow_depth, buf_from.general_shadow_depth, buf_to.general_shadow_depth, total_steps, current_step);
    p_delta_int(&buf.general_shadow_blur, buf_from.general_shadow_blur, buf_to.general_shadow_blur, total_steps, current_step);

    p_delta_int(&buf.color_type, buf_from.color_type, buf_to.color_type, total_steps, current_step);
    p_delta_gdouble(&buf.color_noise, buf_from.color_noise, buf_to.color_noise, total_steps, current_step);


    gimp_set_data("plug_in_gimpressionist", &buf, sizeof(buf)); 

    return 0; /* OK */
}
