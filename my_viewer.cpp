
// CSE170 Completed PA4 - Ritesh Sharma and Marcelo Kallmann

# include "my_viewer.h"

# include <sigogl/ui_button.h>
# include <sigogl/ui_slider.h>
# include <sig/sn_primitive.h>
# include <sig/sn_transform.h>
# include <sig/sn_manipulator.h>

# include <sigogl/ws_run.h>

MyViewer::MyViewer ( int x, int y, int w, int h, const char* l ) : WsViewer(x,y,w,h,l)
{
	_torus_n = 10;
	_torus_R = 1.0f;
	_torus_r = 0.2f;
	_flat=true;
	build_ui ();
	build_scene ();
}

void MyViewer::build_ui ()
{
	UiPanel *p;
	UiManager* uim = WsWindow::uim();
	p = uim->add_panel ( "", UiPanel::HorizLeft );
	p->add ( _slider=new UiSlider ( "Resolution: n=", EvSlider, 0, 0, 200 ) );
	_slider->range ( 3, 50 );
	_slider->format ( 2, 0 );
	_slider->all_events ( true );
	p->add ( new UiButton ( "Exit", EvExit ) ); p->top()->separate();
}

void MyViewer::build_scene ()
{
	_torus = new SnModel;
	_torus->color(GsColor::red);   // using node color (instead of model materials)
	rootg()->add(_torus);

	_normals = new SnLines;
	_normals->visible(false);
	rootg()->add(_normals);

	make_torus(_flat);
}

inline GsVec evaluate_torus_point ( double R, double r, double theta, double phi )
{
	return GsVec ( (R+r*cos(theta))*cos(phi), (R+r*cos(theta))*sin(phi), r*sin(theta) );
}

inline GsVec torus_axis_point ( double R, double r, double theta, double phi )
{
	return GsVec ( R*cos(phi), R*sin(phi), 0.0 );
}

void MyViewer::make_torus ( bool flat )
{
	GsModel* m = _torus->model();  // access model definition class in the node
	m->init();
	double inc = GS_2PI/_torus_n;
	GsVec axisp;

	// (Note: this implementation can still be improved to remove all duplicate vertices!)
	for ( double theta = 0; theta <= GS_2PI; theta = theta + inc )
	{
		m->V.push() = evaluate_torus_point ( _torus_R, _torus_r, theta, 0 );
		m->V.push() = evaluate_torus_point ( _torus_R, _torus_r, theta+inc, 0 );

		if ( !flat )
		{	axisp = torus_axis_point ( _torus_R, _torus_r, theta, 0 );
			m->N.push() = normalize ( m->V.top(1)-axisp );
			m->N.push() = normalize ( m->V.top()-axisp );
		}

		for ( double phi = 0; phi <= GS_2PI; phi = phi + inc )
		{
			m->V.push() = evaluate_torus_point ( _torus_R, _torus_r, theta, phi+inc );
			m->V.push() = evaluate_torus_point ( _torus_R, _torus_r, theta+inc, phi+inc );
			int i = m->V.size()-4;
			m->F.push() = GsModel::Face ( i, i+2, i+3 );
			m->F.push() = GsModel::Face ( i+3, i+1, i ); 

			if ( !flat )
			{	axisp = torus_axis_point ( _torus_R, _torus_r, theta, phi+inc );
				m->N.push() = normalize ( m->V.top(1)-axisp );
				m->N.push() = normalize ( m->V.top()-axisp );
			}
		}
	}

	if ( flat )
	{
		for ( int i=0; i<_torus->model()->F.size(); i++ )
		{
			const GsVec& a = m->V[ m->F[i].a ];
			const GsVec& b = m->V[ m->F[i].b ];
			const GsVec& c = m->V[ m->F[i].c ];
			m->N.push() = cross ( b-a, c-a );
			m->N.top().normalize();
		}
		_torus->model()->set_mode(GsModel::Flat, GsModel::NoMtl);
	}
	else
	{
		m->set_mode(GsModel::Smooth, GsModel::NoMtl);
	}

	// update normals if they are visible:
	if ( _normals->visible() ) make_normals(_flat);
}

void MyViewer::add_texture(bool image) {
	double inc = GS_2PI / _torus_n;

	GsModel& m = *_torus->model();
	GsModel::Group& g = *m.G.push();
	g.fi = 0;
	g.fn = m.F.size();
	g.dmap = new GsModel::Texture;

	int whichcake = texturecakecounter % 3;

	if (whichcake == 0) {
		g.dmap->fname.set("../../rainbow.jpg");
	}
	if (whichcake == 1) {
		g.dmap->fname.set("../../checkers.jpg");
	}
	if (whichcake == 2) {
		g.dmap->fname.set("../../void.jpg");
	}

	m.M.push().init();

	for (double theta = 0; theta <= GS_2PI; theta = theta + inc) {
		m.T.push() = GsPnt2(theta / GS_2PI, 0.0);
		m.T.push() = GsPnt2((theta + inc) / GS_2PI, 0.0);
		for (double phi = 0; phi <= GS_2PI; phi = phi + inc) {
			m.T.push() = GsPnt2(theta / GS_2PI, (phi + inc) / GS_2PI);
			m.T.push() = GsPnt2((theta + inc) / GS_2PI, (phi + inc) / GS_2PI);
		}
	}
	m.set_mode(GsModel::Smooth, GsModel::PerGroupMtl);
	m.textured = true;
}

void MyViewer::make_normals ( bool flat )
{
	float my_scale_factor = 0.15f;
	SnLines* l = _normals;
	l->init();
	l->line_width(2.0f);
	l->color(GsColor::darkgreen);
	if (flat)
	{
		for ( int i = 0; i < _torus->model()->F.size(); i++ )
		{
			const GsVec &a = _torus->model()->V[_torus->model()->F[i].a];
			const GsVec &b = _torus->model()->V[_torus->model()->F[i].b];
			const GsVec &c = _torus->model()->V[_torus->model()->F[i].c];
			GsVec fcenter = (a + b + c) / 3.0f;
			l->push ( fcenter, fcenter + normalize(cross ( b-a, c-a ))*my_scale_factor );
		}
	}
	else
	{
		for ( int i = 0; i < _torus->model()->V.size(); i++ )
		{
			l->push ( _torus->model()->V[i], _torus->model()->V[i] + _torus->model()->N[i]*my_scale_factor );
		}
	}
}

int MyViewer::handle_keyboard ( const GsEvent &e )
{
	int ret = WsViewer::handle_keyboard ( e ); // 1st let system check events
	if ( ret ) return ret;
	GsMat m1, m2, m3;
	GsVec currentposition;
	switch ( e.key )
	{	case GsEvent::KeyEsc : gs_exit(); return 1;

		case 'q': _torus_n++; make_torus(_flat); _slider->value((float)_torus_n); break;
		case 'a': _torus_n--; if(_torus_n<3) _torus_n=3; _slider->value((float)_torus_n); make_torus(_flat); break;

		case 'w': _torus_r += 0.02f; make_torus(_flat); break;
		case 's': _torus_r -= 0.02f; make_torus(_flat); break;
		case 'e': _torus_R += 0.02f; make_torus(_flat); break;
		case 'd': _torus_R -= 0.02f; make_torus(_flat); break;

		case 'z': _flat=true; make_torus(_flat); break;
		case 'x': _flat=false; make_torus(_flat); break;

		case GsEvent::KeySpace: add_texture(true); break;

		case 'c': _normals->visible(true); make_normals(_flat); break;
		case 'v': _normals->visible(false); break;
		default: gsout<<"Key pressed: "<<e.key<<gsnl;
	}
	render();
	return 0;
}

int MyViewer::uievent ( int e )
{
	switch ( e )
	{	case EvSlider: _torus_n=(int)_slider->value(); make_torus(_flat); render(); break;
		case EvExit: gs_exit();
	}
	return WsViewer::uievent(e);
}
