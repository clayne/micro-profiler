//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#include <frontend/piechart.h>

#include <frontend/function_hint.h>

#include <agge/blenders.h>
#include <agge/curves.h>
#include <agge/filling_rules.h>
#include <agge/path.h>
#include <agge/blenders_simd.h>
#include <algorithm>
#include <math.h>

using namespace agge;
using namespace std;
using namespace placeholders;
using namespace wpl;

namespace micro_profiler
{
	namespace
	{
		join<arc, arc> pie_segment(real_t cx, real_t cy, real_t outer_r, real_t inner_r, real_t start, real_t end)
		{	return join<arc, arc>(arc(cx, cy, outer_r, start, end), arc(cx, cy, inner_r, end, start));	}
	}

	void piechart::set_hint(shared_ptr<function_hint> hint)
	{	_hint = hint;	}

	void piechart::set_model(shared_ptr<model_t> m)
	{
		_model = m;
		_invalidate_connection = _model
			? _model->invalidate += bind(&piechart::on_invalidated, this) : slot_connection();
		on_invalidated();
	}

	void piechart::set_selection_model(shared_ptr<wpl::dynamic_set_model> m)
	{
		_selection = m;
		_selection_invalidate_connection = _selection
			? _selection->invalidate += bind(&piechart::on_invalidated, this) : slot_connection();
		on_invalidated();
	}

	void piechart::draw(gcontext &ctx, gcontext::rasterizer_ptr &rasterizer_) const
	{
		typedef blender_solid_color<simd::blender_solid_color, platform_pixel_order> blender;

		real_t start = -pi * 0.5f;

		for (auto i = _segments.begin(); i != _segments.end(); ++i)
		{
			real_t end = start + i->share_angle;
			real_t outer_r = _outer_r * (is_selected(i->index) ? _selection_emphasis_k + 1.0f : 1.0f);

			if (i->share_angle > 0.005)
			{
				add_path(*rasterizer_, pie_segment(_center.x, _center.y, outer_r, _inner_r, start, end));
				rasterizer_->close_polygon();
				ctx(rasterizer_, blender(i->segment_color), winding<>());
			}
			start = end;
		}
	}

	void piechart::layout(const placed_view_appender &append_view, const box<int> &box_)
	{
		_outer_r = 0.5f * real_t((min)(box_.w, box_.h));
		_center.x = _outer_r, _center.y = _outer_r;
		_outer_r /= (1.0f + _selection_emphasis_k);
		_inner_r = 0.5f * _outer_r;
		integrated_control<wpl::control>::layout(append_view, box_);

		if (_hint && _hint->is_active())
		{
			const auto box = _hint->get_box();
			placed_view pv = {
				_hint,
				shared_ptr<native_view>(),
				create_rect(_last_mouse.x, _last_mouse.y + 16, _last_mouse.x + box.w, _last_mouse.y + box.h + 16),
				0,
				true,
			};

			append_view(pv);
		}
	}

	void piechart::mouse_leave() throw()
	{
		if (_hint && _hint->select(npos()))
			layout_changed(true);
	}

	void piechart::mouse_move(int /*depressed*/, int x, int y)
	{
		if (_hint)
		{
			auto index = find_sector(static_cast<real_t>(x), static_cast<real_t>(y));
			index = npos() != index ? _segments[index].index : npos();
			const auto update_hierarchy = _hint->select(find_sector(static_cast<real_t>(x), static_cast<real_t>(y)));

			_last_mouse = create_point(x, y);
			if (_hint->is_active() || update_hierarchy)
				layout_changed(update_hierarchy);
		}
	}

	void piechart::mouse_down(mouse_buttons /*button*/, int depressed, int x, int y)
	{
		const auto idx = find_sector(static_cast<real_t>(x), static_cast<real_t>(y));

		if (!_selection)
			return;
		if (!(depressed & keyboard_input::control))
			_selection->clear();
		if (npos() != idx)
			_selection->add(idx);
	}

	void piechart::mouse_double_click(mouse_buttons /*button*/, int /*depressed*/, int x, int y)
	{
		const auto idx = find_sector(static_cast<real_t>(x), static_cast<real_t>(y));

		if (npos() != idx)
			item_activate(idx);
	}

	void piechart::on_invalidated()
	{
		segment rest = { npos(), 0.0f, 2.0f * pi, _color_rest };
		real_t reciprocal_sum = 0.0f;
		index_type i, count;
		vector<color>::const_iterator j;

		_segments.clear();
		for (i = 0, j = _palette.begin(), count = _model ? _model->get_count() : 0; i != count; ++i)
		{
			double value;

			_model->get_value(i, value);

			segment s = { i, static_cast<real_t>(value), 0.0f, };

			if (j != _palette.end())
			{
				s.segment_color = *j++;
				_segments.push_back(s);
			}
			reciprocal_sum += s.value;
		}
		rest.value = reciprocal_sum;
		reciprocal_sum = reciprocal_sum ? 1.0f / reciprocal_sum : 0.0f;
		for (segments_t::iterator k = _segments.begin(), end = _segments.end(); k != end; ++k)
		{
			const real_t share_angle = 2.0f * pi * k->value * reciprocal_sum;

			k->share_angle = share_angle;
			rest.share_angle -= share_angle;
		}
		_segments.push_back(rest);
		invalidate(nullptr);
	}

	piechart::index_type piechart::find_sector(real_t x, real_t y)
	{
		x -= _center.x, y -= _center.y;
		real_t r = distance(0.0f, 0.0f, x, y), a = 0.5f * pi - static_cast<real_t>(atan2(x, y));

		if (r >= _inner_r)
		{
			real_t start = -pi * 0.5f;

			for (auto i = _segments.begin(); i != _segments.end(); ++i)
			{
				real_t end = start + i->share_angle;

				if (((start <= a) & (a < end))
					&& r < _outer_r * (is_selected(i->index) ? (_selection_emphasis_k + 1.0f) : 1.0f))
				{
					return i->index;
				}
				start = end;
			}
		}
		return npos();
	}

	bool piechart::is_selected(index_type index) const
	{	return index != npos() && _selection && _selection->contains(index);	}
}
