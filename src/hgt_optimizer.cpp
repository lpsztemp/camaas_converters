#include <list>
#include <memory>
#include <algorithm>
#include <vector>
#include <iterator>
#include <cassert>
#include <tuple>
#include <array>
#include <thread>
#include <future>
#include <atomic>
#include <cmath>
#include <basedefs.h>
#include <hgt_optimizer.h>
#include <binary_streams.h>

constexpr static double ADDITIVE_ERROR = 1E-6;

constexpr static bool vertex_has_zero_height(const point_t& pt)
{
	return pt.z >= 0?pt.z <= ADDITIVE_ERROR:pt.z >= -ADDITIVE_ERROR;
}

inline static bool vertex_has_zero_height(unsigned pt);

template <class FaceType>
inline static ConstantDomainDataId GetFaceDomainDataId(const FaceType& face)
{
	for (const auto& pt:face)
	{
		if (!vertex_has_zero_height(pt))
			return ConstantDomainDataId::SurfaceLand;
	}
	return ConstantDomainDataId::SurfaceWater;
}

template <class FaceType>
inline static bool SameDomainData(const FaceType& left, const FaceType& right)
{
	return GetFaceDomainDataId(left) == GetFaceDomainDataId(right);
}
//inline static bool SameDomainData(const Face& left, const Face& right);

class Matrix
{
	const short* m_points = nullptr;
	std::unique_ptr<std::int8_t[]> m_pVertexStatus;
	unsigned short m_cColumns = 0, m_cRows = 0;
	double m_eColumnResolution = 0, m_eRowResolution = 0;
public:
	Matrix() = default;
	Matrix(const short* pPoints, unsigned short cColumns, unsigned short cRows, double eColumnResolution, double eRowResolution)
		:m_points(pPoints), m_pVertexStatus(std::make_unique<std::int8_t[]>(std::size_t(cColumns) * cRows)), 
		m_cColumns(cColumns), m_cRows(cRows), m_eColumnResolution(eColumnResolution), m_eRowResolution(eRowResolution) 
	{
		auto cBlocks = std::thread::hardware_concurrency();
		auto cItemsTotal = unsigned(cColumns) * cRows;
		auto cBlock = (cItemsTotal + cBlocks - 1) / cBlocks;
		std::list<std::thread> threads;
		for (unsigned iThread = 0; iThread < 0; ++iThread)
		{
			threads.emplace_back([this, iThread, cBlock, cItemsTotal](std::int8_t* p) -> void
			{
				auto iEnd = std::min((iThread + 1) * cBlock, cItemsTotal);
				for (auto iElement = iThread * cBlock; iElement < iEnd; ++iElement)
				{
					auto col = this->point_x(iElement);
					auto row = this->point_y(iElement);
					if (col > 0 && col < this->columns() - 1 
						&& row > 0 && row < this->rows() - 1
						&& this->point_z(col, row) - this->point_z(col - 1, row) == this->point_z(col + 1, row) - this->point_z(col, row)
						&& this->point_z(col, row) - this->point_z(col, row - 1) == this->point_z(col, row + 1) - this->point_z(col, row)
						&& this->point_z(col, row - 1) - this->point_z(col - 1, row - 1) == this->point_z(col, row) - this->point_z(col - 1, row)
						&& this->point_z(col, row + 1) - this->point_z(col - 1, row + 1) == this->point_z(col, row) - this->point_z(col - 1, row)
						&& this->point_z(col + 1, row - 1) - this->point_z(col, row - 1) == this->point_z(col + 1, row) - this->point_z(col, row)
						&& this->point_z(col + 1, row + 1) - this->point_z(col, row + 1) == this->point_z(col + 1, row) - this->point_z(col, row))
					{
						point_t v_tl = this->get_external_point(col - 1, row - 1);
						point_t v_tp = this->get_external_point(col, row - 1);
						point_t v_tr = this->get_external_point(col + 1, row - 1);
						point_t v_lf = this->get_external_point(col - 1, row);
						point_t v_md = this->get_external_point(col, row);
						point_t v_rt = this->get_external_point(col + 1, row);
						point_t v_bl = this->get_external_point(col - 1, row + 1);
						point_t v_bt = this->get_external_point(col, row + 1);
						point_t v_br = this->get_external_point(col + 1, row + 1);
						point_t f_tl_lb[] = {v_tl, v_md, v_lf};
						point_t f_tl_rt[] = {v_tl, v_tp, v_md};
						point_t f_tr_lb[] = {v_tp, v_rt, v_md};
						point_t f_tr_rt[] = {v_tp, v_tr, v_rt};
						point_t f_bl_lb[] = {v_lf, v_bt, v_bl};
						point_t f_bl_rt[] = {v_lf, v_md, v_bt};
						point_t f_br_lb[] = {v_md, v_br, v_bt};
						point_t f_br_rt[] = {v_md, v_rt, v_br};
						p[iElement] = std::int8_t(SameDomainData(face_t(std::begin(f_tl_lb), std::end(f_tl_lb)), face_t(std::begin(f_tl_rt), std::end(f_tl_rt)))
							&& SameDomainData(face_t(std::begin(f_tl_rt), std::end(f_tl_rt)), face_t(std::begin(f_tr_lb), std::end(f_tr_lb)))
							&& SameDomainData(face_t(std::begin(f_tr_lb), std::end(f_tr_lb)), face_t(std::begin(f_tr_rt), std::end(f_tr_rt)))
							&& SameDomainData(face_t(std::begin(f_tr_rt), std::end(f_tr_rt)), face_t(std::begin(f_bl_lb), std::end(f_bl_lb)))
							&& SameDomainData(face_t(std::begin(f_bl_lb), std::end(f_bl_lb)), face_t(std::begin(f_bl_rt), std::end(f_bl_rt)))
							&& SameDomainData(face_t(std::begin(f_bl_rt), std::end(f_bl_rt)), face_t(std::begin(f_br_lb), std::end(f_br_lb)))
							&& SameDomainData(face_t(std::begin(f_br_lb), std::end(f_br_lb)), face_t(std::begin(f_br_rt), std::end(f_br_rt))));
					}else
						p[iElement] = false;
				}
			}, m_pVertexStatus.get());
		}
		for (auto& thr:threads)
			thr.join();
	}
	inline unsigned short columns() const noexcept
	{
		return m_cColumns;
	}
	inline unsigned short rows() const noexcept
	{
		return m_cRows;
	}
	bool is_empty_point(unsigned short col, unsigned short row) const noexcept
	{
		return is_empty_point(this->locate(col, row));
	}
	inline bool is_empty_point(unsigned index) const noexcept
	{
		return bool(m_pVertexStatus[index]);
	}
	inline unsigned short point_x(unsigned index) const noexcept
	{
		return index % this->columns();
	}
	inline unsigned short point_y(unsigned index) const noexcept
	{
		return index / this->columns();
	}
	inline short point_z(unsigned index) const noexcept
	{
		return m_points[index];
	}
	inline short point_z(unsigned short col, unsigned short row) const noexcept
	{
		return this->point_z(this->locate(col, row));
	}
	inline unsigned locate(unsigned short col, unsigned short row) const noexcept
	{
		return row * this->columns() + col;
	}
	inline point_t get_external_point(unsigned short col, unsigned short row) const
	{
		return {double(col) * m_eColumnResolution, double(row) * m_eRowResolution, double(point_z(col, row))};
	}
	inline point_t get_external_point(unsigned index) const
	{
		return {double(this->point_x(index)) * m_eColumnResolution, double(this->point_y(index)) * m_eRowResolution, double(point_z(index))};
	}
};

Matrix g_matrix;

class Face
{
public:
	typedef unsigned value_type; //index pointing to the matrix element
	typedef unsigned* pointer;
	typedef const unsigned* const_pointer;
	typedef unsigned& reference;
	typedef const unsigned& const_reference;
	typedef unsigned* iterator;
	typedef const unsigned* const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	typedef unsigned size_type;
	typedef signed difference_type;
private:
	static constexpr size_type SMALL_NUMBER = 4;
	union
	{
		pointer m_vertices_big;
		value_type m_vertices_small[SMALL_NUMBER];
	};
	pointer m_pVertices;
	size_type m_vertices_count = 0;
public:
	explicit operator face_t() const;
	inline value_type point(size_type in_face_index) const noexcept
	{
		return m_pVertices[in_face_index];
	}
	inline size_type size() const noexcept
	{
		return m_vertices_count;
	}
	inline iterator begin() noexcept {return &m_pVertices[0];}
	inline const_iterator cbegin() const noexcept {return const_cast<const unsigned*>(&m_pVertices[0]);}
	inline const_iterator begin() const noexcept {return this->cbegin();}
	inline iterator end() noexcept {return &m_pVertices[m_vertices_count];}
	inline const_iterator cend() const noexcept {return const_cast<const unsigned*>(&m_pVertices[m_vertices_count]);}
	inline const_iterator end() const noexcept {return this->cend();}
	inline reverse_iterator rbegin() noexcept {return std::make_reverse_iterator(this->end());}
	inline const_reverse_iterator crbegin() const noexcept {return std::make_reverse_iterator(this->end());}
	inline const_reverse_iterator rbegin() const noexcept {return std::make_reverse_iterator(this->end());}
	inline reverse_iterator rend() noexcept {return std::make_reverse_iterator(this->begin());}
	inline const_reverse_iterator crend() const noexcept {return std::make_reverse_iterator(this->begin());}
	inline const_reverse_iterator rend() const noexcept {return std::make_reverse_iterator(this->begin());}

	bool is_complanar_to(const Face& right) const
	{
		auto n1 = cross_product(g_matrix.get_external_point(m_pVertices[1]) - g_matrix.get_external_point(m_pVertices[0]),
			g_matrix.get_external_point(m_pVertices[2]) - g_matrix.get_external_point(m_pVertices[1]));
		assert(fabs(n1.x) > ADDITIVE_ERROR || fabs(n1.y) > ADDITIVE_ERROR || fabs(n1.z) > ADDITIVE_ERROR);
		auto n2 = cross_product(g_matrix.get_external_point(right.m_pVertices[1]) - g_matrix.get_external_point(right.m_pVertices[0]),
			g_matrix.get_external_point(right.m_pVertices[2]) - g_matrix.get_external_point(right.m_pVertices[1]));
		assert(fabs(n2.x) > ADDITIVE_ERROR || fabs(n2.y) > ADDITIVE_ERROR || fabs(n2.z) > ADDITIVE_ERROR);
		auto res = cross_product(n1, n2);
		return fabs(res.x) <= ADDITIVE_ERROR && fabs(res.y) <= ADDITIVE_ERROR && fabs(res.z) <= ADDITIVE_ERROR;
	}
	bool can_unite_with_quadruplet_left_edge(unsigned short quad_left_col, unsigned short quad_top_row) const;
	bool can_unite_with_quadruplet_top_edge(unsigned short quad_left_col, unsigned short quad_top_row) const;
	bool can_unite_with_quadruplet_right_edge(unsigned short quad_left_col, unsigned short quad_top_row) const;
	bool can_unite_with_quadruplet_bottom_edge(unsigned short quad_left_col, unsigned short quad_top_row) const;
	bool can_unite_with(const Face& right) const
	{
		return this->is_complanar_to(right) && SameDomainData(*this, right);
	}
private:
	class ReversedConstructor;
public:
	Face() = default;
	class Constructor
	{
		std::list<unsigned> m_lstEdgeVertices;
	public:
		Constructor() = default;
		void add_point(unsigned short col, unsigned short row) noexcept
		{
			assert(!g_matrix.is_empty_point(col, row));
			if (this->size() >= 2)
			{
				auto it_pp = m_lstEdgeVertices.rbegin();
				auto it_p = it_pp++;
				if ((g_matrix.point_y(*it_p) - g_matrix.point_y(*it_pp)) * (col - g_matrix.point_x(*it_p))
					== (row - g_matrix.point_y(*it_p)) * (g_matrix.point_x(*it_p) - g_matrix.point_x(*it_pp)))
					*it_p = g_matrix.locate(col, row);
				else
					m_lstEdgeVertices.emplace_back(g_matrix.locate(col, row));
			}else
				m_lstEdgeVertices.emplace_back(g_matrix.locate(col, row));
		}
		void add_list(const Face& face)
		{
			auto it = face.begin();
			auto pt = *it++;
			this->add_point(g_matrix.point_x(pt), g_matrix.point_y(pt));
			while (it != face.end())
				m_lstEdgeVertices.emplace_back(*it);
		}
		void add_list(Face::Constructor&& right)
		{
			if (!right.empty())
			{
				if (m_lstEdgeVertices.empty())
					m_lstEdgeVertices = std::move(right.m_lstEdgeVertices);
				else
				{
					auto it_p = std::prev(m_lstEdgeVertices.end());
					m_lstEdgeVertices.splice(m_lstEdgeVertices.end(), std::move(right.m_lstEdgeVertices));
					auto it_n = it_p--;
					auto it = it_n++;
					if (it_n != m_lstEdgeVertices.end() 
						&& (g_matrix.point_y(*it) - g_matrix.point_y(*it_p)) * (g_matrix.point_x(*it_n) - g_matrix.point_x(*it))
						== (g_matrix.point_y(*it_n) - g_matrix.point_y(*it)) * (g_matrix.point_x(*it) - g_matrix.point_x(*it_p)))
						m_lstEdgeVertices.erase(it);
				}
			}
		}
		typedef Face::ReversedConstructor Reversed;
		void add_reversed_list(Reversed&& right);
		//returns an index of the next quadruplet to analyze
		unsigned add_face_to_the_right(unsigned short col, unsigned short row);
		void add_face_to_the_bottom(unsigned short col, unsigned short row);
		auto begin() {return m_lstEdgeVertices.begin();}
		auto begin() const {return m_lstEdgeVertices.begin();}
		auto cbegin() const {return m_lstEdgeVertices.cbegin();}
		auto end() {return m_lstEdgeVertices.end();}
		auto end() const {return m_lstEdgeVertices.end();}
		auto cend() const {return m_lstEdgeVertices.cend();}
		auto rbegin() {return m_lstEdgeVertices.rbegin();}
		auto rbegin() const {return m_lstEdgeVertices.rbegin();}
		auto crbegin() const {return m_lstEdgeVertices.crbegin();}
		auto rend() {return m_lstEdgeVertices.rend();}
		auto rend() const {return m_lstEdgeVertices.rend();}
		auto crend() const {return m_lstEdgeVertices.crend();}
		unsigned size() const {return unsigned(m_lstEdgeVertices.size());}
		bool empty() const {return m_lstEdgeVertices.empty();}
	};
	Face(Constructor&& constr):Face(std::move(constr), int()) {}
	//MSVC bug with fold expressions: https://developercommunity.visualstudio.com/content/problem/301623/fold-expressions-in-template-instantiations-fail-t.html
	template <class ... Points, class = std::enable_if_t<(sizeof ... (Points) > 1) && (sizeof ... (Points) <= SMALL_NUMBER) && std::conjunction_v<std::is_integral<Points> ...>>>
	explicit Face(Points ... vertices):m_vertices_small{vertices...}, m_pVertices(m_vertices_small), m_vertices_count(unsigned(sizeof...(Points)))
	{
		static_assert(sizeof...(Points) >= 3, "Invalid number of vertices specified for a face");
		/*assert((g_matrix.point_y(m_pVertices[sizeof...(Points) - 2]) - g_matrix.point_y(m_pVertices[sizeof...(Points) - 3])) 
			* (g_matrix.point_x(m_pVertices[sizeof...(Points) - 1]) - g_matrix.point_x(m_pVertices[sizeof...(Points) - 2]))
			== (g_matrix.point_y(m_pVertices[sizeof...(Points) - 1]) - g_matrix.point_y(m_pVertices[sizeof...(Points) - 2])) 
			* (g_matrix.point_x(m_pVertices[sizeof...(Points) - 2]) - g_matrix.point_x(m_pVertices[sizeof...(Points) - 3])));*/
	}
	template <class ... Points, class = void, class = std::enable_if_t<(sizeof ... (Points) > SMALL_NUMBER) && std::conjunction_v<std::is_integral<Points> ...>>>
	explicit Face(Points ... vertices):Face(std::array<typename std::tuple_element<0, std::tuple<Points...>>::type, sizeof ... (Points)>{vertices...}, int()) {}
	Face(const Face&) = delete;
	Face(Face&& right)
	{
		*this = std::move(right);
	}
	Face& operator=(const Face&) = delete;
	Face& operator=(Face&& right)
	{
		if (this == &right)
			return *this;
		if (m_vertices_count > SMALL_NUMBER)
			delete [] m_vertices_big;
		if (right.m_vertices_count <= SMALL_NUMBER)
		{
			std::copy(&right.m_vertices_small[0], &right.m_vertices_small[right.m_vertices_count], &m_vertices_small[0]);
			m_pVertices = m_vertices_small;
		}else
		{
			m_vertices_big = right.m_vertices_big;
			m_pVertices = m_vertices_big;
		}
		m_vertices_count = right.m_vertices_count;
		right.m_vertices_count = 0;
		return *this;
	}
	~Face() noexcept
	{
		if (m_vertices_count > SMALL_NUMBER)
			delete [] m_vertices_big;
	}
private:
	class ReversedConstructor
	{
		Face::Constructor m_list;
	public:
		void add_point(unsigned short col, unsigned short row)
		{
			m_list.add_point(col, row); 
		}
		auto begin() {return m_list.begin();}
		auto begin() const {return m_list.begin();}
		auto cbegin() const {return m_list.cbegin();}
		auto end() {return m_list.end();}
		auto end() const {return m_list.end();}
		auto cend() const {return m_list.cend();}
		auto rbegin() {return m_list.rbegin();}
		auto rbegin() const {return m_list.rbegin();}
		auto crbegin() const {return m_list.crbegin();}
		auto rend() {return m_list.rend();}
		auto rend() const {return m_list.rend();}
		auto crend() const {return m_list.crend();}
		unsigned size() const {return unsigned(m_list.size());}
		bool empty() const {return m_list.empty();}
	};
	template <class Container>
	Face(Container&& constr, int) noexcept
	{
		assert(constr.size() >= 3);
		auto it_end = std::end(constr);
		{
			auto it_pp = std::rbegin(constr);
			auto it_p = it_pp++;
			auto col = g_matrix.point_x(*std::begin(constr));
			auto row = g_matrix.point_y(*std::begin(constr));
			if ((g_matrix.point_y(*it_p) - g_matrix.point_y(*it_pp)) * (col - g_matrix.point_x(*it_p))
				== (row - g_matrix.point_y(*it_p)) * (g_matrix.point_x(*it_p) - g_matrix.point_x(*it_pp)))
			{
				assert(constr.size() - 1 >= 3);
				--it_end;
				m_vertices_count = unsigned(constr.size() - 1);
			}else
				m_vertices_count = unsigned(constr.size());
		}
		if (m_vertices_count <= SMALL_NUMBER)
			m_pVertices = m_vertices_small;
		else
		{
			m_vertices_big = new unsigned [m_vertices_count];
			m_pVertices = m_vertices_big;
		}
		//auto it_min = std::min_element(std::begin(constr), it_end);
		//std::move(std::begin(constr), it_min, std::move(it_min, it_end, &m_pVertices[0]));
		std::move(std::begin(constr), it_end, &m_pVertices[0]);
	}
};

void Face::Constructor::add_reversed_list(Face::Constructor::Reversed&& right)
{
	for (auto it = std::rbegin(right); it != std::rend(right); ++it)
		this->add_point(g_matrix.point_x(*it), g_matrix.point_y(*it));
}

struct vertex_converting_iterator
{
	typedef std::input_iterator_tag iterator_category;
	typedef point_t value_type;
	typedef const point_t *pointer, &reference;
	typedef unsigned size_type;
	typedef signed difference_type;
	reference operator*() const
	{
		if (!m_fLastVal)
		{
			m_ptLastVal = g_matrix.get_external_point(*m_it);
			m_fLastVal = true;
		}
		return m_ptLastVal;
	}
	pointer operator->() const
	{
		if (!m_fLastVal)
		{
			m_ptLastVal = g_matrix.get_external_point(*m_it);
			m_fLastVal = true;
		}
		return &m_ptLastVal;
	}
	vertex_converting_iterator& operator++()
	{
		++m_it;
		return *this;
	}
	vertex_converting_iterator operator++(int)
	{
		auto old = *this;
		++*this;
		return old;
	}
	bool operator==(const vertex_converting_iterator& right) const
	{
		return m_it == right.m_it;
	}
	bool operator!=(const vertex_converting_iterator& right) const
	{
		return m_it != right.m_it;
	}
	vertex_converting_iterator() = default;
	explicit vertex_converting_iterator(Face::const_iterator it):m_it(it) {}
	vertex_converting_iterator(const vertex_converting_iterator& right):m_fLastVal(right.m_fLastVal), m_it(right.m_it)
	{
		if (m_fLastVal)
			m_ptLastVal = right.m_ptLastVal;
	}
	vertex_converting_iterator(vertex_converting_iterator&& right):vertex_converting_iterator(right) {}
	vertex_converting_iterator& operator=(const vertex_converting_iterator& right)
	{
		m_fLastVal = right.m_fLastVal;
		m_it = right.m_it;
		if (m_fLastVal)
			m_ptLastVal = right.m_ptLastVal;
		return *this;
	}
	vertex_converting_iterator& operator=(vertex_converting_iterator&& right)
	{
		return *this = right;
	}
private:
	mutable bool m_fLastVal = false;
	mutable point_t m_ptLastVal;
	Face::const_iterator m_it;

};

Face::operator face_t() const
{
	return face_t(vertex_converting_iterator(this->begin()), vertex_converting_iterator(this->end()), this->size());
}

//bool SameDomainData(const Face& left, const Face& right)
//{
//	return SameDomainData(face_t(left), face_t(right));
//}

inline static bool vertex_has_zero_height(unsigned pt)
{
	return g_matrix.point_z(pt) == 0;
}

struct FaceCompareLess
{
	inline bool operator()(const Face& left, const Face& right) const noexcept
	{
		return std::lexicographical_compare(std::begin(left), std::end(left), std::begin(right), std::end(right));
	}
};

struct FaceCompareEqual
{
	inline bool operator()(const Face& left, const Face& right) const noexcept
	{
		return std::equal(std::begin(left), std::end(left), std::begin(right), std::end(right));
	}
};

class FaceSet
{
	std::vector<Face> m_vFaces;
public:
	struct Constructor
	{
		Constructor()
		{
			m_vFaces.reserve((g_matrix.columns() - 1) * (g_matrix.rows() - 1) * 2);
		}
		Face& add_face(Face&& face)
		{
			m_vFaces.emplace_back(std::move(face));
			return *m_vFaces.rbegin();
		}
	private:
		friend class FaceSet;
		std::vector<Face> m_vFaces;
	};
	FaceSet() = default;
	FaceSet(Constructor&& constr):m_vFaces(std::move(constr.m_vFaces))
	{
		m_vFaces.shrink_to_fit();
#ifndef NDEBUG
		if (m_vFaces.empty())
			return;
		std::sort(m_vFaces.begin(), m_vFaces.end(), FaceCompareLess());
		FaceCompareEqual eq;
		for (auto it = m_vFaces.begin(), it_n = std::next(m_vFaces.begin()); it_n != m_vFaces.end(); it = it_n++)
		{
			if (eq(*it, *it_n))
				throw std::invalid_argument("Non-unique face is found in the input set"); //replace to it_n = m_vFaces.erase(++it, ++it_n);
		}
#endif
	}
	auto begin() noexcept {return m_vFaces.begin();}
	auto begin() const noexcept {return m_vFaces.begin();}
	auto cbegin() const noexcept {return m_vFaces.cbegin();}
	auto end() noexcept {return m_vFaces.end();}
	auto end() const noexcept {return m_vFaces.end();}
	auto cend() const noexcept {return m_vFaces.cend();}
	auto size() const noexcept {return m_vFaces.size();}
	bool empty() const noexcept {return m_vFaces.empty();}
};

/*
3 2
x x
x x
0 1

 0) o o     1) o o     2) o o     3) o o
    o o        . o        o .        . .
 4) o .     5) o .     6) o .     7) o .
    o o        . o        o .        . .
 8) . o     9) . o    10) . o    11) . o
    o o        . o        o .        . .
12) . .    13) . .    14) . .    15) . .
    o o        . o        o .        . .
*/
unsigned char GetPointQuadrupletType(unsigned short quad_left_col, unsigned short quad_top_row)
{
	assert(quad_left_col < g_matrix.columns() && quad_top_row < g_matrix.rows());
	return 
		(unsigned char(!g_matrix.is_empty_point(quad_left_col, quad_top_row)) << 3)		| (unsigned char(!g_matrix.is_empty_point(quad_left_col + 1, quad_top_row)) << 2) |
		(unsigned char(!g_matrix.is_empty_point(quad_left_col, quad_top_row + 1)) << 0)	| (unsigned char(!g_matrix.is_empty_point(quad_left_col + 1, quad_top_row + 1)) << 1);
}

bool Face::can_unite_with_quadruplet_left_edge(unsigned short quad_left_col, unsigned short quad_top_row) const
{
	assert(quad_left_col < g_matrix.columns() - 1 && quad_top_row < g_matrix.rows() - 1);
	switch (GetPointQuadrupletType(quad_left_col, quad_top_row))
	{
	case 11:
	{
		Face cur_face = Face{g_matrix.locate(quad_left_col, quad_top_row), g_matrix.locate(quad_left_col + 1, quad_top_row + 1), g_matrix.locate(quad_left_col, quad_top_row + 1)};
		return this->can_unite_with(cur_face);
	}
	case 13:
	{
		Face cur_face = Face{g_matrix.locate(quad_left_col, quad_top_row), g_matrix.locate(quad_left_col + 1, quad_top_row), g_matrix.locate(quad_left_col, quad_top_row + 1)};
		if (!this->can_unite_with(cur_face))
			return false;
		if (quad_top_row > 0)
			return !cur_face.can_unite_with_quadruplet_bottom_edge(quad_left_col, quad_top_row - 1);
		return true;
	}
	case 15:
	{
		Face face_lb = Face{g_matrix.locate(quad_left_col, quad_top_row), g_matrix.locate(quad_left_col + 1, quad_top_row + 1), g_matrix.locate(quad_left_col, quad_top_row + 1)};
		if (!this->can_unite_with(face_lb))
			return false;
		if (quad_top_row < g_matrix.rows() - 2 && face_lb.can_unite_with_quadruplet_top_edge(quad_left_col, quad_top_row + 1))
			return false;
		Face face_rt = Face{g_matrix.locate(quad_left_col, quad_top_row), g_matrix.locate(quad_left_col + 1, quad_top_row), g_matrix.locate(quad_left_col + 1, quad_top_row + 1)};
		return !face_lb.can_unite_with(face_rt) || quad_top_row == 0 || !face_rt.can_unite_with_quadruplet_bottom_edge(quad_left_col, quad_top_row - 1);
	}
	default:
		return false;
	};
}

bool Face::can_unite_with_quadruplet_top_edge(unsigned short quad_left_col, unsigned short quad_top_row) const
{
	assert(quad_left_col < g_matrix.columns() - 1 && quad_top_row < g_matrix.rows() - 1);
	switch (GetPointQuadrupletType(quad_left_col, quad_top_row))
	{
	case 13:
	{
		Face cur_face = Face{g_matrix.locate(quad_left_col, quad_top_row), g_matrix.locate(quad_left_col + 1, quad_top_row), g_matrix.locate(quad_left_col, quad_top_row + 1)};
		return this->can_unite_with(cur_face);
	}
	case 14:
	case 15:
	{
		Face cur_face = Face{g_matrix.locate(quad_left_col, quad_top_row), g_matrix.locate(quad_left_col + 1, quad_top_row), g_matrix.locate(quad_left_col + 1, quad_top_row + 1)};
		return this->can_unite_with(cur_face);
	}
	default:
		return false;
	};
}

bool Face::can_unite_with_quadruplet_right_edge(unsigned short quad_left_col, unsigned short quad_top_row) const
{
	assert(quad_left_col < g_matrix.columns() - 1 && quad_top_row < g_matrix.rows() - 1);
	switch (GetPointQuadrupletType(quad_left_col, quad_top_row))
	{
	case 7:
	{
		Face cur_face = Face{g_matrix.locate(quad_left_col + 1, quad_top_row), g_matrix.locate(quad_left_col + 1, quad_top_row + 1), g_matrix.locate(quad_left_col, quad_top_row + 1)};
		return this->can_unite_with(cur_face);
	}
	case 14:
	case 15:
	{
		Face face_rt = Face{g_matrix.locate(quad_left_col, quad_top_row), g_matrix.locate(quad_left_col + 1, quad_top_row), g_matrix.locate(quad_left_col + 1, quad_top_row + 1)};
		if (!this->can_unite_with(face_rt))
			return false;
		if (quad_top_row > 0 && face_rt.can_unite_with_quadruplet_bottom_edge(quad_left_col, quad_top_row - 1))
			return false;
		Face face_lb = Face{g_matrix.locate(quad_left_col, quad_top_row), g_matrix.locate(quad_left_col + 1, quad_top_row + 1), g_matrix.locate(quad_left_col, quad_top_row + 1)};
		return !face_lb.can_unite_with(face_rt) || quad_top_row >= g_matrix.rows() - 2 || !face_lb.can_unite_with_quadruplet_top_edge(quad_left_col, quad_top_row + 1);
	}
	default:
		return false;
	};
}

bool Face::can_unite_with_quadruplet_bottom_edge(unsigned short quad_left_col, unsigned short quad_top_row) const
{
	assert(quad_left_col < g_matrix.columns() - 1 && quad_top_row < g_matrix.rows() - 1);
	switch (GetPointQuadrupletType(quad_left_col, quad_top_row))
	{
	case 7:
	{
		Face cur_face = Face{g_matrix.locate(quad_left_col + 1, quad_top_row), g_matrix.locate(quad_left_col + 1, quad_top_row + 1), g_matrix.locate(quad_left_col, quad_top_row + 1)};
		return this->can_unite_with(cur_face);
	}
	case 11:
	case 15:
	{
		Face cur_face = Face{g_matrix.locate(quad_left_col, quad_top_row), g_matrix.locate(quad_left_col + 1, quad_top_row + 1), g_matrix.locate(quad_left_col, quad_top_row + 1)};
		return this->can_unite_with(cur_face);
	}
	default:
		return false;
	};
}

//returns an index of the next quadruplet to analyze
unsigned Face::Constructor::add_face_to_the_right(unsigned short col, unsigned short row)
{
	Face::Constructor::Reversed constrEnd;
	unsigned short cur_col = col;
	unsigned ret_index;
	do
	{
		switch (GetPointQuadrupletType(cur_col, row))
		{
		case 11:
			this->add_point(cur_col + 1, row + 1);
			ret_index = g_matrix.locate(cur_col, row) + 1;
			break;
		case 13:
			this->add_point(cur_col + 1, row);
			ret_index = g_matrix.locate(cur_col, row) + 1;
			break;
		case 15:
		{
			Face lb_face = Face{g_matrix.locate(cur_col, row), g_matrix.locate(cur_col + 1, row + 1), g_matrix.locate(cur_col, row + 1)};
			Face rt_face = Face{g_matrix.locate(cur_col, row), g_matrix.locate(cur_col + 1, row), g_matrix.locate(cur_col + 1, row + 1)};
			if (lb_face.can_unite_with(rt_face) && (row == 0 || !rt_face.can_unite_with_quadruplet_bottom_edge(cur_col, row - 1)))
			{
				this->add_point(cur_col + 1, row);
				constrEnd.add_point(cur_col + 1, row + 1);
				if (cur_col < g_matrix.columns() - 2 && rt_face.can_unite_with_quadruplet_left_edge(cur_col + 1, row))
				{
					++cur_col;
					continue;
				}
				ret_index = g_matrix.locate(cur_col, row) + 1;
			}else
			{
				this->add_point(cur_col + 1, row + 1);
				ret_index = g_matrix.locate(cur_col, row);
			}
			break;
		}
		default:
#ifndef NDEBUG
			assert(false);
#endif
			ret_index = g_matrix.locate(g_matrix.columns() - 1, g_matrix.rows() - 1) + 1;
		}
		break;
	}while (true);
	this->add_reversed_list(std::move(constrEnd));
	return ret_index;
}

void Face::Constructor::add_face_to_the_bottom(unsigned short col, unsigned short row)
{
	Face::Constructor::Reversed constrEnd;
	unsigned short cur_row = row;
	unsigned char cur_type = GetPointQuadrupletType(col, cur_row);
	do
	{
		switch (cur_type)
		{
		case 13:
			this->add_point(col, cur_row + 1);
			break;
		case 14:
			this->add_point(col + 1, cur_row + 1);
			break;
		case 15:
		{
			Face rt_face = Face{g_matrix.locate(col, cur_row), g_matrix.locate(col + 1, cur_row), g_matrix.locate(col + 1, cur_row + 1)};
			Face lb_face = Face{g_matrix.locate(col, cur_row), g_matrix.locate(col + 1, cur_row + 1), g_matrix.locate(col, cur_row + 1)};
			if (lb_face.can_unite_with(rt_face))
			{
				this->add_point(col + 1, cur_row + 1);
				constrEnd.add_point(col, cur_row + 1);
				if (cur_row < g_matrix.rows() - 2 && lb_face.can_unite_with_quadruplet_top_edge(col, cur_row + 1))
				{
					cur_type = GetPointQuadrupletType(col, ++cur_row);
					continue;
				}
				break;
			}else
				this->add_point(col + 1, cur_row + 1);
			break;
		}
#ifndef NDEBUG
		default:
			assert(false);
#endif
		}
		break;
	}while (true);
	this->add_reversed_list(std::move(constrEnd));
}

//returned object will be empty, if a vertex V is encountered for which V < g_matrix.locate(v1_col, v1_row)) is true
Face::Constructor obtain_big_face(unsigned short v1_col, unsigned short v1_row, unsigned short v2_col, unsigned short v2_row)
{
	unsigned start = g_matrix.locate(v1_col, v1_row);
	Face::Constructor constr;
	unsigned short cur_col = v1_col;
	unsigned short cur_row = v1_row;
	unsigned short next_col = v2_col;
	unsigned short next_row = v2_row;
	unsigned next_index = g_matrix.locate(next_col, next_row);
	constr.add_point(v1_col, v1_row);
	do
	{
		if (next_index < start)
			return Face::Constructor();
		if (next_row == cur_row + 1)
		{
			if (next_col == cur_col + 1)
			{
				if (!g_matrix.is_empty_point(next_col - 1, next_row + 1))
				{
					cur_col = next_col--;
					cur_row = next_row++;
				}else if (!g_matrix.is_empty_point(next_col, next_row + 1))
				{
					cur_col = next_col;
					cur_row = next_row++;
				}else if (!g_matrix.is_empty_point(next_col + 1, next_row + 1))
				{
					cur_col = next_col++;
					cur_row = next_row++;
				}else if (!g_matrix.is_empty_point(next_col + 1, next_row))
				{
					cur_col = next_col++;
					cur_row = next_row;
				}else if (!g_matrix.is_empty_point(next_col + 1, next_row - 1))
				{
					cur_col = next_col++;
					cur_row = next_row--;
				}else //if (!g_matrix.is_empty_point(next_col, next_row - 1))
				{
					assert(!g_matrix.is_empty_point(next_col, next_row - 1));
					cur_col = next_col;
					cur_row = next_row--;
				}
			}else if (next_col == cur_col)
			{
				if (!g_matrix.is_empty_point(next_col - 1, next_row))
				{
					cur_col = next_col--;
					cur_row = next_row;
				}else if (!g_matrix.is_empty_point(next_col - 1, next_row + 1))
				{
					cur_col = next_col--;
					cur_row = next_row++;
				}else if (!g_matrix.is_empty_point(next_col, next_row + 1))
				{
					cur_col = next_col;
					cur_row = next_row++;
				}else if (!g_matrix.is_empty_point(next_col + 1, next_row + 1))
				{
					cur_col = next_col++;
					cur_row = next_row++;
				}else if (!g_matrix.is_empty_point(next_col + 1, next_row))
				{
					cur_col = next_col++;
					cur_row = next_row;
				}else //if (!g_matrix.is_empty_point(next_col + 1, next_row - 1))
				{
					assert(!g_matrix.is_empty_point(next_col + 1, next_row - 1));
					cur_col = next_col++;
					cur_row = next_row--;
				}
			}else
			{
				assert(next_col == cur_col - 1);
				if (!g_matrix.is_empty_point(next_col - 1, next_row - 1))
				{
					cur_col = next_col--;
					cur_row = next_row--;
				}else if (!g_matrix.is_empty_point(next_col - 1, next_row))
				{
					cur_col = next_col--;
					cur_row = next_row;
				}else if (!g_matrix.is_empty_point(next_col - 1, next_row + 1))
				{
					cur_col = next_col--;
					cur_row = next_row++;
				}else if (!g_matrix.is_empty_point(next_col, next_row + 1))
				{
					cur_col = next_col;
					cur_row = next_row++;
				}else if (!g_matrix.is_empty_point(next_col + 1, next_row + 1))
				{
					cur_col = next_col++;
					cur_row = next_row++;
				}else //if (!g_matrix.is_empty_point(next_col + 1, next_row))
				{
					assert(!g_matrix.is_empty_point(next_col + 1, next_row));
					cur_col = next_col++;
					cur_row = next_row;
				}
			}
		}else if (next_row == cur_row)
		{
			if (next_col == cur_col + 1)
			{
				if (!g_matrix.is_empty_point(next_col, next_row + 1))
				{
					cur_col = next_col;
					cur_row = next_row++;
				}else if (!g_matrix.is_empty_point(next_col + 1, next_row + 1))
				{
					cur_col = next_col++;
					cur_row = next_row++;
				}else if (!g_matrix.is_empty_point(next_col + 1, next_row))
				{
					cur_col = next_col++;
					cur_row = next_row;
				}else if (!g_matrix.is_empty_point(next_col + 1, next_row - 1))
				{
					cur_col = next_col++;
					cur_row = next_row--;
				}else if (!g_matrix.is_empty_point(next_col, next_row - 1))
				{
					cur_col = next_col;
					cur_row = next_row--;
				}else //if (!g_matrix.is_empty_point(next_col - 1, next_row - 1))
				{
					assert(!g_matrix.is_empty_point(next_col - 1, next_row - 1));
					cur_col = next_col--;
					cur_row = next_row--;
				}
			}else
			{
				assert(next_col == cur_col - 1);
				if (!g_matrix.is_empty_point(next_col, next_row - 1))
				{
					cur_col = next_col;
					cur_row = next_row--;
				}else if (!g_matrix.is_empty_point(next_col - 1, next_row - 1))
				{
					cur_col = next_col--;
					cur_row = next_row--;
				}else if (!g_matrix.is_empty_point(next_col - 1, next_row))
				{
					cur_col = next_col--;
					cur_row = next_row;
				}else if (!g_matrix.is_empty_point(next_col - 1, next_row + 1))
				{
					cur_col = next_col--;
					cur_row = next_row++;
				}else if (!g_matrix.is_empty_point(next_col, next_row + 1))
				{
					cur_col = next_col;
					cur_row = next_row++;
				}else //if (!g_matrix.is_empty_point(next_col + 1, next_row + 1))
				{
					assert((!g_matrix.is_empty_point(next_col + 1, next_row + 1)));
					cur_col = next_col++;
					cur_row = next_row++;
				}
			}
		}else
		{
			assert(next_row == cur_row - 1);
			if (next_col == cur_col + 1)
			{
				if (!g_matrix.is_empty_point(next_col + 1, next_row + 1))
				{
					cur_col = next_col++;
					cur_row = next_row++;
				}else if (!g_matrix.is_empty_point(next_col + 1, next_row))
				{
					cur_col = next_col++;
					cur_row = next_row;
				}else if (!g_matrix.is_empty_point(next_col + 1, next_row - 1))
				{
					cur_col = next_col++;
					cur_row = next_row--;
				}else if (!g_matrix.is_empty_point(next_col, next_row - 1))
				{
					cur_col = next_col;
					cur_row = next_row--;
				}else if (!g_matrix.is_empty_point(next_col - 1, next_row - 1))
				{
					cur_col = next_col--;
					cur_row = next_row--;
				}else // if (!g_matrix.is_empty_point(next_col - 1, next_row))
				{
					assert(!g_matrix.is_empty_point(next_col - 1, next_row));
					cur_col = next_col--;
					cur_row = next_row;
				}
			}else if (next_col == cur_col)
			{
				if (!g_matrix.is_empty_point(next_col + 1, next_row))
				{
					cur_col = next_col++;
					cur_row = next_row;
				}else if (!g_matrix.is_empty_point(next_col + 1, next_row - 1))
				{
					cur_col = next_col++;
					cur_row = next_row--;
				}else if (!g_matrix.is_empty_point(next_col, next_row - 1))
				{
					cur_col = next_col;
					cur_row = next_row--;
				}else if (!g_matrix.is_empty_point(next_col - 1, next_row - 1))
				{
					cur_col = next_col--;
					cur_row = next_row--;
				}else if (!g_matrix.is_empty_point(next_col - 1, next_row))
				{
					cur_col = next_col--;
					cur_row = next_row;
				}else //if (!g_matrix.is_empty_point(next_col - 1, next_row + 1))
				{
					assert(!g_matrix.is_empty_point(next_col - 1, next_row + 1));
					cur_col = next_col--;
					cur_row = next_row++;
				}
			}else
			{
				assert(next_col == cur_col - 1);
				if (!g_matrix.is_empty_point(next_col + 1, next_row - 1))
				{
					cur_col = next_col++;
					cur_row = next_row--;
				}else if (!g_matrix.is_empty_point(next_col, next_row - 1))
				{
					cur_col = next_col;
					cur_row = next_row--;
				}else if (!g_matrix.is_empty_point(next_col - 1, next_row - 1))
				{
					cur_col = next_col--;
					cur_row = next_row--;
				}else if (!g_matrix.is_empty_point(next_col - 1, next_row))
				{
					cur_col = next_col--;
					cur_row = next_row;
				}else if (!g_matrix.is_empty_point(next_col - 1, next_row + 1))
				{
					cur_col = next_col--;
					cur_row = next_row++;
				}else //if (!g_matrix.is_empty_point(next_col, next_row + 1))
				{
					assert(!g_matrix.is_empty_point(next_col, next_row + 1));
					cur_col = next_col;
					cur_row = next_row++;
				}
			}
		}
		constr.add_point(cur_col, cur_row);
	}while ((next_index = g_matrix.locate(next_col, next_row)) != start);
	return constr;
}

//returns an index of the last quadruplet included in the face
unsigned parse_vertex(FaceSet::Constructor& faces, unsigned short col, unsigned short row)
{
	assert(col < g_matrix.columns() && row < g_matrix.rows());
	if (col == g_matrix.columns() - 1)
		return g_matrix.locate(col, row) + 1;
	if (row == g_matrix.rows() - 1)
		return g_matrix.columns() * g_matrix.rows();
	switch (GetPointQuadrupletType(col, row))
	{
	case 7:
	{
		Face cur_face = Face{g_matrix.locate(col + 1, row), g_matrix.locate(col + 1, row + 1), g_matrix.locate(col, row + 1)};
		if (row < g_matrix.rows() - 2 && cur_face.can_unite_with_quadruplet_top_edge(col, row + 1))
		{
			Face::Constructor constr;
			constr.add_point(col + 1, row);
			constr.add_point(col + 1, row + 1);
			constr.add_face_to_the_bottom(col, row + 1);
			constr.add_point(col, row + 1);
			faces.add_face(Face(std::move(constr)));
			return g_matrix.locate(col, row) + 1;
		}else if (col < g_matrix.columns() - 2 && cur_face.can_unite_with_quadruplet_left_edge(col + 1, row))
		{
			Face::Constructor constr;
			constr.add_point(col + 1, row);
			auto ret = constr.add_face_to_the_right(col + 1, row);
			constr.add_point(col + 1, row + 1);
			constr.add_point(col, row + 1);
			faces.add_face(Face(std::move(constr)));
			return ret;
		}else
		{
			faces.add_face(std::move(cur_face));
			return g_matrix.locate(col, row) + 1;
		}
	}
	case 10:
	{
		if (g_matrix.is_empty_point(col - 1, row + 1))
			return g_matrix.locate(col, row) + 1;
		auto constr = obtain_big_face(col, row, col + 1, row + 1);
		if (!constr.empty())
			faces.add_face(std::move(constr));
		return g_matrix.locate(col, row) + 1;
	}
	case 11:
	{
		Face cur_face = Face{g_matrix.locate(col, row), g_matrix.locate(col + 1, row + 1), g_matrix.locate(col, row + 1)};
		if (row < g_matrix.rows() - 2 && cur_face.can_unite_with_quadruplet_top_edge(col, row + 1))
		{
			Face::Constructor constr;
			constr.add_point(col, row);
			constr.add_point(col + 1, row + 1);
			constr.add_face_to_the_bottom(col, row + 1);
			constr.add_point(col, row + 1);
			faces.add_face(Face(std::move(constr)));
		}else if (col == 0 || !cur_face.can_unite_with_quadruplet_right_edge(col - 1, row))
			faces.add_face(std::move(cur_face));
		return g_matrix.locate(col, row) + 1;
	}
	case 12:
	{
		if (g_matrix.is_empty_point(col - 1, row + 1))
			return g_matrix.locate(col, row) + 1;
		auto constr = obtain_big_face(col, row, col + 1, row);
		if (!constr.empty())
			faces.add_face(std::move(constr));
		return g_matrix.locate(col, row) + 1;
	}
	case 13:
	{
		Face cur_face = Face{g_matrix.locate(col, row), g_matrix.locate(col + 1, row), g_matrix.locate(col, row + 1)};
		if ((col == 0 || !cur_face.can_unite_with_quadruplet_right_edge(col - 1, row))
			&& (row == 0 || !cur_face.can_unite_with_quadruplet_bottom_edge(col, row - 1)))
			faces.add_face(std::move(cur_face));
		return g_matrix.locate(col, row) + 1;
	}
	case 14:
	{
		if (!g_matrix.is_empty_point(col - 1, row + 1))
		{
			auto constr = obtain_big_face(col, row, col + 1, row + 1);
			if (!constr.empty())
				faces.add_face(std::move(constr));
		}
		Face cur_face = Face{g_matrix.locate(col, row), g_matrix.locate(col + 1, row), g_matrix.locate(col + 1, row + 1)};
		if (row > 0 && cur_face.can_unite_with_quadruplet_bottom_edge(col, row - 1))
			return g_matrix.locate(col, row) + 1;
		if (col < g_matrix.columns() - 2 && cur_face.can_unite_with_quadruplet_left_edge(col + 1, row))
		{
			Face::Constructor constr;
			constr.add_point(col, row);
			constr.add_point(col + 1, row);
			auto ret = constr.add_face_to_the_right(col + 1, row);
			constr.add_point(col + 1, row + 1);
			faces.add_face(Face(std::move(constr)));
			return ret;
		}else
		{
			faces.add_face(std::move(cur_face));
			return g_matrix.locate(col, row) + 1;
		}
	}
	case 15:
	{
		Face lb_face = Face{g_matrix.locate(col, row), g_matrix.locate(col + 1, row + 1), g_matrix.locate(col, row + 1)};
		Face rt_face = Face{g_matrix.locate(col, row), g_matrix.locate(col + 1, row), g_matrix.locate(col + 1, row + 1)};
		if (lb_face.can_unite_with(rt_face))
		{
			if (row > 0 && rt_face.can_unite_with_quadruplet_bottom_edge(col, row - 1))
				return g_matrix.locate(col, row) + 1;
			if (row < g_matrix.rows() - 2 && lb_face.can_unite_with_quadruplet_top_edge(col, row + 1))
			{
				Face::Constructor constr;
				constr.add_point(col, row);
				constr.add_point(col + 1, row);
				constr.add_point(col + 1, row + 1);
				constr.add_face_to_the_bottom(col, row + 1);
				constr.add_point(col, row + 1);
				faces.add_face(Face(std::move(constr)));
				return g_matrix.locate(col, row) + 1;
			}
			if (col > 0 && lb_face.can_unite_with_quadruplet_right_edge(col - 1, row))
				return g_matrix.locate(col, row) + 1;
			if (col == g_matrix.columns() - 2 || !rt_face.can_unite_with_quadruplet_left_edge(col + 1, row))
			{
				faces.add_face(Face{g_matrix.locate(col, row), g_matrix.locate(col + 1, row), g_matrix.locate(col + 1, row + 1), g_matrix.locate(col, row + 1)});
				return g_matrix.locate(col, row) + 1;
			}else
			{
				Face::Constructor constr;
				constr.add_point(col, row);
				constr.add_point(col + 1, row);
				auto ret = constr.add_face_to_the_right(col + 1, row);
				constr.add_point(col + 1, row + 1);
				constr.add_point(col, row + 1);
				faces.add_face(Face(std::move(constr)));
				return ret;
			}
		}else
		{
			if (row < g_matrix.rows() - 2 && lb_face.can_unite_with_quadruplet_top_edge(col, row + 1))
			{
				Face::Constructor constr;
				constr.add_point(col, row);
				constr.add_point(col + 1, row + 1);
				constr.add_face_to_the_bottom(col, row + 1);
				constr.add_point(col, row + 1);
				faces.add_face(Face(std::move(constr)));
			}else if (col == 0 || !lb_face.can_unite_with_quadruplet_right_edge(col - 1, row))
				faces.add_face(std::move(lb_face));
			if (row == 0 || !rt_face.can_unite_with_quadruplet_bottom_edge(col, row - 1))
			{
				if (col < g_matrix.columns() - 2 && rt_face.can_unite_with_quadruplet_left_edge(col + 1, row))
				{
					Face::Constructor constr;
					constr.add_point(col, row);
					constr.add_point(col + 1, row);
					auto ret = constr.add_face_to_the_right(col + 1, row);
					constr.add_point(col + 1, row + 1);
					faces.add_face(Face(std::move(constr)));
					return ret;
				}else
				{
					faces.add_face(std::move(rt_face));
					return g_matrix.locate(col, row) + 1;
				}
			}else
				return g_matrix.locate(col, row) + 1;
		}
	}
	default:
		return g_matrix.locate(col, row) + 1;
	}
}

FaceSet parse_matrix(unsigned start_element /*= 0*/, unsigned end_element /*= g_matrix.columns() * g_matrix.rows()*/)
{
	FaceSet::Constructor set;
	for (unsigned index = start_element; index < end_element; )
	{
		index = parse_vertex(set, g_matrix.point_x(index), g_matrix.point_y(index));
	}
	return set;
}

#include <ostream>

static std::atomic_flag g_run = ATOMIC_FLAG_INIT;

unsigned convert_hgt_to_index_based_face(binary_ostream& os, const short* pInput, unsigned short cColumns, unsigned short cRows, double eColumnResolution, double eRowResolution)
{
	std::list<std::future<FaceSet>> futures;
	while (g_run.test_and_set(std::memory_order_acquire))
		continue;
	g_matrix = Matrix(pInput, cColumns, cRows, eColumnResolution, eRowResolution);
	//auto cBlocks = unsigned(1);
	auto cBlocks = unsigned(std::thread::hardware_concurrency());
	auto cItemsTotal = unsigned(cColumns) * cRows;
	auto cBlock = (cItemsTotal + cBlocks - 1) / cBlocks;
	for (unsigned i = 0; i < cBlocks; ++i)
		futures.emplace_back(std::async(std::launch::async, [i, cBlock, cItemsTotal]() -> auto
		{
			return parse_matrix(i * cBlock, std::min((i + 1) * cBlock, cItemsTotal));
		}));
	unsigned face_count = 0;
	auto face_count_pos = os.tellp();
	os.seekp(sizeof(unsigned), std::ios_base::cur);
	for (auto& fut:futures)
	{
		auto set = fut.get();
		face_count += unsigned(set.size());
		for (const auto& face:set)
		{
			os << face.size();
			for (auto pt:face)
				os << g_matrix.point_x(pt) << g_matrix.point_y(pt) << g_matrix.point_z(pt);
		}
	}
	g_run.clear(std::memory_order_release);
	auto endp = os.tellp();
	os.seekp(face_count_pos);
	os << face_count;
	os.seekp(endp);
	return face_count;
}

#if CPP17_FILESYSTEM_SUPPORT
unsigned convert_hgt_to_index_based_face(std::filesystem::path input, std::filesystem::path output)
{
	auto os = binary_fostream(output);
	std::ifstream is(input, std::ios_base::in | std::ios_base::binary);
	is.seekg(0, std::ios_base::end);
	auto cb = is.tellg();
	if (cb % sizeof(short) != 0)
		throw std::invalid_argument("Unexpected HGT file size");
	is.seekg(0, std::ios_base::beg);
	switch (cb)
	{
	case HGT_1.cColumns * HGT_1.cRows * sizeof(short):
	{
		auto pInput = std::make_unique<short[]>(cb / sizeof(short));
		is.read(reinterpret_cast<char*>(pInput.get()), cb);
		return convert_hgt_to_index_based_face(os, pInput.get(), unsigned(HGT_1.cColumns), unsigned(HGT_1.cRows), HGT_1.dx, HGT_1.dy);
	}
	case HGT_3.cColumns * HGT_3.cRows * sizeof(short):
	{
		auto pInput = std::make_unique<short[]>(cb / sizeof(short));
		is.read(reinterpret_cast<char*>(pInput.get()), cb);
		return convert_hgt_to_index_based_face(os, pInput.get(), unsigned(HGT_3.cColumns), unsigned(HGT_3.cRows), HGT_3.dx, HGT_3.dy);
	}
	default:
		throw std::invalid_argument("Unexpected HGT file size");
	};
}
#endif //CPP17_FILESYSTEM_SUPPORT

struct conversion_result
{
	conversion_result() = default;
	conversion_result(short min_height, short max_height, std::vector<face_t>&& lstWater, std::vector<face_t>&& lstLand)
		:m_min_height(min_height), m_max_height(max_height), m_WaterFaces(std::move(lstWater)), m_LandFaces(std::move(lstLand)) {}

	inline short min_height() const noexcept
	{
		return m_min_height;
	}
	inline short max_height() const noexcept
	{
		return m_max_height;
	}
	inline std::vector<face_t>& water_faces() noexcept
	{
		return m_WaterFaces;
	}
	inline std::vector<face_t>& land_faces() noexcept
	{
		return m_LandFaces;
	}
	inline const std::vector<face_t>& water_faces() const noexcept
	{
		return m_WaterFaces;
	}
	inline const std::vector<face_t>& land_faces() const noexcept
	{
		return m_LandFaces;
	}
private:
	short m_min_height, m_max_height;
	std::vector<face_t> m_WaterFaces;
	std::vector<face_t> m_LandFaces;
};

struct hgt_state
{
	void start(binary_ostream& os, const IDomainConverter& converter, unsigned points_to_process_at_start)
	{
		assert(m_pOs == nullptr && m_faces.empty() && process_land_face_ptr == nullptr && process_water_face_ptr == nullptr);
		m_pOs = &os;
		m_face_water_domain_data = converter.constant_face_domain_data(ConstantDomainDataId::SurfaceWater);
		m_poly_water_domain_data = converter.constant_poly_domain_data(ConstantDomainDataId::SurfaceWater);
		m_face_land_domain_data = converter.constant_face_domain_data(ConstantDomainDataId::SurfaceLand);
		m_poly_land_domain_data = converter.constant_poly_domain_data(ConstantDomainDataId::SurfaceLand);
		auto internal_set = parse_matrix(0, points_to_process_at_start);
		process_land_face_ptr = &hgt_state::write_land_face_and_poly_header_to_stream;
		process_water_face_ptr = &hgt_state::write_water_face_and_poly_header_to_stream;
		std::vector<face_t> stored_faces;
		stored_faces.reserve(internal_set.size());
		m_face_count = unsigned(internal_set.size());
		for (auto& internal_face:internal_set)
		{
			for (auto pt:internal_face)
			{
				auto height = g_matrix.point_z(pt);
				if (height < m_min_height)
					m_min_height = height;
				if (height > m_max_height)
					m_max_height = height;
			}
			switch (GetFaceDomainDataId(internal_face))
			{
			case ConstantDomainDataId::SurfaceLand:
			{
				process_land_face(face_t(std::move(internal_face)));
				break;
			}
			case ConstantDomainDataId::SurfaceWater:
				stored_faces.emplace_back(face_t(std::move(internal_face)));
				break;
			default:
				throw std::invalid_argument("Invalid face domain data in HGT");
			}
		}
		stored_faces.shrink_to_fit();
		m_faces.emplace_back(std::move(stored_faces));
	}
	void add_results(conversion_result&& res)
	{
		m_face_count += unsigned(res.land_faces().size());
		for (auto& face:res.land_faces())
			process_land_face(std::move(face));
		res.land_faces().clear();
		m_faces.emplace_back(std::move(res.water_faces()));
		if (res.min_height() < m_min_height)
			m_min_height = res.min_height();
		if (res.max_height() > m_max_height)
			m_max_height = res.max_height();
	}
	HGT_CONVERSION_STATS finalize()
	{
		if (m_face_count != 0)
		{
			m_pOs->seekp(m_face_count_pos);
			*m_pOs << unsigned(m_face_count);
			m_pOs->seekp(0, std::ios_base::end);
		}
		for (auto& vWater:m_faces)
		{
			for (auto& face:vWater)
				this->process_water_face(std::move(face));
		}
		if (m_face_count != 0)
		{
			m_pOs->seekp(m_face_count_pos);
			*m_pOs << unsigned(m_face_count);
			m_pOs->seekp(0, std::ios_base::end);
		}
		HGT_CONVERSION_STATS res{m_min_height, m_max_height, m_objects};
		*this = hgt_state();
		return res;
	}
private:
	short m_min_height = std::numeric_limits<short>::max(), m_max_height = std::numeric_limits<short>::min();
	unsigned m_objects = 0;
	binary_ostream* m_pOs = nullptr;
	binary_ostream::pos_type m_face_count_pos = 0;
	unsigned m_face_count = 0;
	std::list<std::vector<face_t>> m_faces;
	void (hgt_state::*process_land_face_ptr)(face_t&& face) = nullptr;
	void (hgt_state::*process_water_face_ptr)(face_t&& face) = nullptr;
	domain_data_map m_face_water_domain_data, m_poly_water_domain_data, m_face_land_domain_data, m_poly_land_domain_data;

	void write_poly_header(const std::string_view& poly_name, const domain_data_map& domain_data)
	{
		*m_pOs << std::uint32_t(poly_name.size());
		m_pOs->write(poly_name.data(), poly_name.size());
		*m_pOs << std::uint32_t(domain_data.size());
		for (auto& prDomain:domain_data)
		{
			*m_pOs << std::uint32_t(prDomain.first.size());
			m_pOs->write(prDomain.first.data(), prDomain.first.size());
			*m_pOs << std::uint32_t(prDomain.second.size());
			m_pOs->write(prDomain.second.data(), prDomain.second.size());
		}
		*m_pOs << ObjectPoly;
		m_face_count_pos = m_pOs->tellp();
	}
	inline void write_land_poly_header()
	{
		this->write_poly_header("HGT land", m_poly_land_domain_data);
	}
	void write_water_poly_header()
	{
		this->write_poly_header("HGT water", m_poly_water_domain_data);
	}
	void write_face_to_stream(face_t&& face, const domain_data_map& domain_data)
	{
		*m_pOs << std::uint32_t(face.size());
		for (auto& pt:face)
			*m_pOs << std::uint32_t(3) << pt.x << pt.y << pt.z;
		*m_pOs << std::uint32_t(domain_data.size());
		for (auto& prDomain:domain_data)
		{
			*m_pOs << std::uint32_t(prDomain.first.size());
			m_pOs->write(prDomain.first.data(), prDomain.first.size());
			*m_pOs << std::uint32_t(prDomain.second.size());
			m_pOs->write(prDomain.second.data(), prDomain.second.size());
		}
	}
	void write_water_face_to_stream(face_t&& face)
	{
		this->write_face_to_stream(std::move(face), m_face_water_domain_data);
	}
	void write_land_face_to_stream(face_t&& face)
	{
		this->write_face_to_stream(std::move(face), m_face_land_domain_data);
	}
	void write_water_face_and_poly_header_to_stream(face_t&& face)
	{
		++m_objects;
		this->write_water_poly_header();
		this->write_water_face_to_stream(std::move(face));
		process_water_face_ptr = &hgt_state::write_water_face_to_stream;
	}
	void write_land_face_and_poly_header_to_stream(face_t&& face)
	{
		++m_objects;
		this->write_land_poly_header();
		this->write_land_face_to_stream(std::move(face));
		process_land_face_ptr = &hgt_state::write_land_face_to_stream;
	}
	inline void process_land_face(face_t&& face)
	{
		return (this->*process_land_face_ptr)(std::move(face));
	}
	inline void process_water_face(face_t&& face)
	{
		return (this->*process_water_face_ptr)(std::move(face));
	}
};

static HGT_CONVERSION_STATS convert_hgt_to_external_poly_set(binary_ostream& os, const short* pInput, unsigned short cColumns, unsigned short cRows, 
	double eColumnResolution, double eRowResolution, const IDomainConverter& converter)
{
	std::list<std::future<conversion_result>> futures;
	while (g_run.test_and_set(std::memory_order_acquire))
		continue;
	g_matrix = Matrix(pInput, cColumns, cRows, eColumnResolution, eRowResolution);
	//auto cBlocks = unsigned(1);
	auto cBlocks = unsigned(std::thread::hardware_concurrency());
	auto cItemsTotal = unsigned(cColumns) * cRows;
	auto cBlock = (cItemsTotal + cBlocks - 1) / cBlocks;
	for (unsigned i = 1; i < cBlocks; ++i)
		futures.emplace_back(std::async(std::launch::async, [i, cBlock, cItemsTotal]() -> auto
		{
			std::vector<face_t> land_faces, water_faces;
			unsigned start_element = i * cBlock;
			unsigned end_element = std::min(start_element + cBlock, cItemsTotal);
			auto internal_set = parse_matrix(start_element, end_element);
			auto max_height = std::numeric_limits<short>::min();
			auto min_height = std::numeric_limits<short>::max();
			land_faces.reserve(internal_set.size());
			water_faces.reserve(internal_set.size());
			for (auto& internal_face:internal_set)
			{
				for (auto pt:internal_face)
				{
					auto height = g_matrix.point_z(pt);
					if (height < min_height)
						min_height = height;
					if (height > max_height)
						max_height = height;
				}
				switch (GetFaceDomainDataId(internal_face))
				{
				case ConstantDomainDataId::SurfaceLand:
					land_faces.emplace_back(face_t(std::move(internal_face)));
					break;
				case ConstantDomainDataId::SurfaceWater:
					water_faces.emplace_back(face_t(std::move(internal_face)));
					break;
				default:
					throw std::invalid_argument("Invalid face domain data in HGT");
				}
			}
			water_faces.shrink_to_fit();
			land_faces.shrink_to_fit();
			return conversion_result(min_height, max_height, std::move(water_faces), std::move(land_faces));
		}));
	hgt_state face_converter;
	face_converter.start(os, converter, std::min(cBlock, cItemsTotal));
	for (auto& fut:futures)
		face_converter.add_results(fut.get());
	g_run.clear(std::memory_order_release);
	return face_converter.finalize();
}

HGT_CONVERSION_STATS convert_hgt(const HGT_RESOLUTION_DATA& resolution, std::istream& is_data, const IDomainConverter& converter, binary_ostream& os)
{
	is_data.seekg(0, std::ios_base::end);
	auto cb = is_data.tellg();
	if (cb % sizeof(short) != 0)
		throw std::invalid_argument("Unexpected HGT file size");
	is_data.seekg(0, std::ios_base::beg);
	switch (cb)
	{
	case HGT_1.cColumns * HGT_1.cRows * sizeof(short):
	{
		auto pInput = std::make_unique<short[]>(cb / sizeof(short));
		is_data.read(reinterpret_cast<char*>(pInput.get()), cb);
		return convert_hgt_to_external_poly_set(os, pInput.get(), unsigned(HGT_1.cColumns), unsigned(HGT_1.cRows), HGT_1.dx, HGT_1.dy, converter);
	}
	case HGT_3.cColumns * HGT_3.cRows * sizeof(short):
	{
		auto pInput = std::make_unique<short[]>(cb / sizeof(short));
		is_data.read(reinterpret_cast<char*>(pInput.get()), cb);
		return convert_hgt_to_external_poly_set(os, pInput.get(), unsigned(HGT_3.cColumns), unsigned(HGT_3.cRows), HGT_3.dx, HGT_3.dy, converter);
	}
	default:
		throw std::invalid_argument("Unexpected HGT file size");
	};
}