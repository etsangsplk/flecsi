/*~--------------------------------------------------------------------------~*
 *  @@@@@@@@  @@           @@@@@@   @@@@@@@@ @@
 * /@@/////  /@@          @@////@@ @@////// /@@
 * /@@       /@@  @@@@@  @@    // /@@       /@@
 * /@@@@@@@  /@@ @@///@@/@@       /@@@@@@@@@/@@
 * /@@////   /@@/@@@@@@@/@@       ////////@@/@@
 * /@@       /@@/@@//// //@@    @@       /@@/@@
 * /@@       @@@//@@@@@@ //@@@@@@  @@@@@@@@ /@@
 * //       ///  //////   //////  ////////  //
 *
 * Copyright (c) 2016 Los Alamos National Laboratory, LLC
 * All rights reserved
 *~--------------------------------------------------------------------------~*/

#include <map>
#include <unordered_map>
#include <vector>
#include <array>
#include <map>
#include <cmath>
#include <bitset>
#include <algorithm>
#include <cassert>
#include <iostream>

namespace flecsi{
namespace ntree_dev{

using entity_id_t = uint64_t;

using entity_id_vec_t = std::vector<entity_id_t>;

template<typename T, size_t D>
class branch_id{
public:
  using int_t = T;

  static const size_t dimension = D;

  static constexpr size_t bits = sizeof(int_t) * 8;

  static constexpr size_t max_depth = (bits - 1)/dimension;

  branch_id()
  : id_(0){}

  branch_id(const std::array<int_t, dimension>& coords)
  : id_(int_t(1) << bits - 1){  
    
    for(size_t i = 0; i < max_depth; ++i){
      for(size_t j = 0; j < dimension; ++j){
        id_ |= (coords[j] & int_t(1) << i) << i + j;
      }
    }
  }

  constexpr branch_id(int_t x, int_t y){
    id_ = table2d_[y >> 8] << 17 | table2d_[x >> 8] << 16 |
      table2d_[y & 0xFF] << 1 | table2d_[x & 0xFF];
  }

  constexpr branch_id(const branch_id& bid)
  : id_(bid.id_){}

  static constexpr branch_id root(){
    return branch_id(int_t(1) << dimension - 1);
  }

  static constexpr branch_id null(){
    return branch_id(0);
  }

  constexpr bool is_null() const{
    return id_ == int_t(0);
  }

  constexpr size_t depth() const{
    int_t id = id_;
    size_t d = 0;
    
    while(id >>= dimension){
      ++d;
    }

    return d;
  }

  constexpr branch_id& operator=(const branch_id& bid){
    id_ = bid.id_;
    return *this;
  }

  constexpr bool operator==(const branch_id& bid) const{
    return id_ == bid.id_;
  }

  constexpr void push(int_t bits){
    assert(bits < int_t(1) << dimension);

    id_ <<= dimension;
    id_ |= bits;
  }

  constexpr void pop(){
    assert(depth() > 0);
    id_ >>= dimension;
  }

  constexpr void pop(size_t d){
    assert(d >= depth());
    id_ >>= d * dimension;
  }

  constexpr branch_id parent() const{
    return branch_id(id_ >> dimension);
  }

  constexpr void truncate(size_t to_depth){
    size_t d = depth();
    
    if(d < to_depth){
      return;
    }

    id_ >>= (d - to_depth) * dimension; 
  }

  void output_(std::ostream& ostr) const{
    constexpr int_t mask = ((int_t(1) << dimension) - 1) << bits - dimension;

    size_t d = max_depth;

    int_t id = id_;

    while((id & mask) == int_t(0)){
      --d;
      id <<= dimension;
    }

    for(size_t i = 0; i <= d; ++i){
      int_t val = (id & mask) >> (bits - dimension);
      ostr << i << ":" << std::bitset<D>(val) << " ";
      id <<= dimension;
    }
  }

  int_t value_() const{
    return id_;
  }

private:
  int_t id_;

  constexpr branch_id(int_t id)
  : id_(id){}

  static constexpr int_t table2d_[256] = {
    0x0000, 0x0001, 0x0004, 0x0005, 0x0010, 0x0011, 0x0014, 0x0015, 
    0x0040, 0x0041, 0x0044, 0x0045, 0x0050, 0x0051, 0x0054, 0x0055, 
    0x0100, 0x0101, 0x0104, 0x0105, 0x0110, 0x0111, 0x0114, 0x0115, 
    0x0140, 0x0141, 0x0144, 0x0145, 0x0150, 0x0151, 0x0154, 0x0155, 
    0x0400, 0x0401, 0x0404, 0x0405, 0x0410, 0x0411, 0x0414, 0x0415, 
    0x0440, 0x0441, 0x0444, 0x0445, 0x0450, 0x0451, 0x0454, 0x0455, 
    0x0500, 0x0501, 0x0504, 0x0505, 0x0510, 0x0511, 0x0514, 0x0515, 
    0x0540, 0x0541, 0x0544, 0x0545, 0x0550, 0x0551, 0x0554, 0x0555, 
    0x1000, 0x1001, 0x1004, 0x1005, 0x1010, 0x1011, 0x1014, 0x1015, 
    0x1040, 0x1041, 0x1044, 0x1045, 0x1050, 0x1051, 0x1054, 0x1055, 
    0x1100, 0x1101, 0x1104, 0x1105, 0x1110, 0x1111, 0x1114, 0x1115, 
    0x1140, 0x1141, 0x1144, 0x1145, 0x1150, 0x1151, 0x1154, 0x1155, 
    0x1400, 0x1401, 0x1404, 0x1405, 0x1410, 0x1411, 0x1414, 0x1415, 
    0x1440, 0x1441, 0x1444, 0x1445, 0x1450, 0x1451, 0x1454, 0x1455, 
    0x1500, 0x1501, 0x1504, 0x1505, 0x1510, 0x1511, 0x1514, 0x1515, 
    0x1540, 0x1541, 0x1544, 0x1545, 0x1550, 0x1551, 0x1554, 0x1555, 
    0x4000, 0x4001, 0x4004, 0x4005, 0x4010, 0x4011, 0x4014, 0x4015, 
    0x4040, 0x4041, 0x4044, 0x4045, 0x4050, 0x4051, 0x4054, 0x4055, 
    0x4100, 0x4101, 0x4104, 0x4105, 0x4110, 0x4111, 0x4114, 0x4115, 
    0x4140, 0x4141, 0x4144, 0x4145, 0x4150, 0x4151, 0x4154, 0x4155, 
    0x4400, 0x4401, 0x4404, 0x4405, 0x4410, 0x4411, 0x4414, 0x4415, 
    0x4440, 0x4441, 0x4444, 0x4445, 0x4450, 0x4451, 0x4454, 0x4455, 
    0x4500, 0x4501, 0x4504, 0x4505, 0x4510, 0x4511, 0x4514, 0x4515, 
    0x4540, 0x4541, 0x4544, 0x4545, 0x4550, 0x4551, 0x4554, 0x4555, 
    0x5000, 0x5001, 0x5004, 0x5005, 0x5010, 0x5011, 0x5014, 0x5015, 
    0x5040, 0x5041, 0x5044, 0x5045, 0x5050, 0x5051, 0x5054, 0x5055, 
    0x5100, 0x5101, 0x5104, 0x5105, 0x5110, 0x5111, 0x5114, 0x5115, 
    0x5140, 0x5141, 0x5144, 0x5145, 0x5150, 0x5151, 0x5154, 0x5155, 
    0x5400, 0x5401, 0x5404, 0x5405, 0x5410, 0x5411, 0x5414, 0x5415, 
    0x5440, 0x5441, 0x5444, 0x5445, 0x5450, 0x5451, 0x5454, 0x5455, 
    0x5500, 0x5501, 0x5504, 0x5505, 0x5510, 0x5511, 0x5514, 0x5515, 
    0x5540, 0x5541, 0x5544, 0x5545, 0x5550, 0x5551, 0x5554, 0x5555
  };
};

template<typename T, size_t D>
std::ostream& operator<<(std::ostream& ostr, const branch_id<T,D>& id){
  id.output_(ostr);
  return ostr;
}

template<typename T, size_t D>
struct branch_id_hasher__{
  size_t operator()(const branch_id<T, D>& k) const{
    return std::hash<T>()(k.value_());
  }
};

double uniform(){
  return double(rand())/RAND_MAX;
}

double uniform(double a, double b){
  return a + (b - a) * uniform();
}

template<typename T, size_t D>
class coordinates{
public:
  using element_t = T;

  static const size_t dimension = D;

  coordinates(){}

  coordinates(std::initializer_list<element_t> il){
    size_t i = 0;
    for(auto v : il){
      pos_[i++] = v;
    }
  }

  coordinates& operator=(const coordinates& c){
    pos_ = c.pos_;
    return *this;
  }

  bool operator==(const coordinates& c) const{
    for(size_t i = 0; i < dimension; ++i){
      if(pos_[i] != c.pos_[i]){
        return false;
      }
    }

    return true;
  }

  element_t operator[](const size_t i) const{
    return pos_[i];
  }

  element_t& operator[](const size_t i){
    return pos_[i];
  }

  element_t distance(const coordinates& u){
    element_t d = 0;
    
    for(size_t i = 0; i < dimension; ++i){
      element_t di = pos_[i] - u.pos_[i];
      d += di * di;
    }
    
    return sqrt(d);
  }

  void print(){
    std::cout << "(";
    for(size_t i = 0; i < dimension; ++i){
      std::cout << pos_[i] << ",";
    }
    std::cout << ")" << std::endl;   
  }

private:
  std::array<element_t, dimension> pos_;
};

enum class action : uint64_t{
  none = 0b00,
  refine = 0b01,
  coarsen = 0b10
};

template<class P>
class ntree : public P{
public:
  using Policy = P;

  static const size_t dimension = Policy::dimension;

  
  using point_t = typename Policy::point_t;

  using element_t = typename Policy::point_t::element_t;


  using branch_int_t = typename Policy::branch_int_t;

  using branch_id_t = branch_id<branch_int_t, dimension>;

  using branch_id_vector_t = std::vector<branch_id_t>;


  using branch_t = typename Policy::branch_t;

  using branch_vector_t = std::vector<branch_t*>;


  using entity_t = typename Policy::entity_t;

  using entity_vector_t = std::vector<entity_t*>;

  ntree(std::initializer_list<element_t> bounds){
    assert(bounds.size() == bounds_.size());

    size_t i = 0;
    for(element_t ei : bounds){
      bounds_[i++] = ei;
    }

    branch_id_t bid = branch_id_t::root();
    auto b = make_branch(bid);
    branch_map_.emplace(bid, b);

    max_depth_ = 0;
  }

  branch_t* find_parent_(branch_id_t bid){
    for(;;){
      auto itr = branch_map_.find(bid);
      if(itr != branch_map_.end()){
        return itr->second;
      }
      bid.pop();
    }
  }

  branch_t* find_parent(branch_t* b){
    return find_parent(b->id());
  }

  branch_t* find_parent(branch_id_t bid){
    return find_parent(bid, max_depth_);
  }

  branch_t* find_parent(branch_id_t bid, size_t max_depth){
    branch_id_t pid = bid;
    pid.truncate(max_depth);

    return find_parent_(pid);
  }

  void insert(entity_t* ent, size_t max_depth){
    branch_id_t bid = to_branch_id(ent->coordinates());

    branch_t* b = find_parent(bid, max_depth);
    ent->set_branch_id_(b->id());

    b->insert(ent);

    switch(b->requested_action()){
      case action::none:
        break;
      case action::refine:
        refine_(*b);
        break;
      default:
        assert(false && "invalid action");
    }
  }

  void insert(entity_t* ent){
    insert(ent, max_depth_);
  }

  void remove(entity_t* ent){
    assert(!ent->branch_id().is_null());

    auto itr = branch_map_.find(ent->branch_id());
    assert(itr != branch_map_.end());
    branch_t* b = itr->second;
    
    b->remove(ent);
    ent->set_branch_id_(branch_id_t::null());

    switch(b->requested_action()){
      case action::none:
        break;
      case action::coarsen:
        coarsen_(*b);
        break;
      case action::refine:
        b->reset();
        break;
      default:
        assert(false && "invalid action");
    }
  }

  void refine_(branch_t& l){
    branch_id_t pid = l.id();
    size_t depth = pid.depth() + 1;

    branch_int_t n = branch_int_t(1) << dimension;
    
    for(branch_int_t bi = 0; bi < n; ++bi){
      branch_id_t bid = pid;
      bid.push(bi);
      auto b = make_branch(bid);
      branch_map_.emplace(bid, b);
    }

    for(auto ent : l){
      insert(ent, depth);
    }

    l.clear();
    l.reset();

    max_depth_ = std::max(max_depth_, depth);
  }

  void coarsen_(branch_t& l){    
    branch_id_t id = l.id();
    branch_id_t pid = id.parent();

    auto itr = branch_map_.find(pid);
    assert(itr != branch_map_.end());
    auto p = itr->second;

    size_t depth = pid.depth();

    constexpr branch_int_t n = branch_int_t(1) << dimension;

    for(branch_int_t ci = 0; ci < n; ++ci){
      branch_id_t cid = pid;
      cid.push(ci);

      auto citr = branch_map_.find(cid);
      assert(citr != branch_map_.end());
      auto c = citr->second;

      for(auto ent : *c){
        p->insert(ent);
        ent->set_branch_id_(pid);
      }

      delete c;
      branch_map_.erase(citr);
    }

    p->reset();

    l.reset();
  }

  branch_vector_t neighbors(branch_t* l) const{
    assert(false && "unimplemented");
  }

  branch_id_vector_t neighbors(branch_id_t b) const{
    assert(false && "unimplemented");
  }

  entity_vector_t locality(entity_t* ent, element_t dist){
    assert(false && "unimplemented");
  }

  branch_t* make_branch(branch_id_t id){
    auto b = new branch_t;
    b->set_id_(id);
    return b;
  }

  template<class... Args>
  entity_t* make_entity(Args&&... args){
    auto ent = new entity_t(std::forward<Args>(args)...);
    return ent;
  }

  branch_id_t to_branch_id(const point_t& p){    
    std::array<branch_int_t, dimension> coords;
    
    for(size_t i = 0; i < dimension; ++i){
      element_t start = bounds_[i * 2];
      element_t end = bounds_[i * 2 + 1];

      coords[i] = (p[i] - start)/(end - start) * 
        (branch_int_t(1) << (branch_id_t::bits - 1)/dimension);
    }

    return branch_id_t(coords);
  }

  size_t max_depth() const{
    return max_depth_;
  }

private:
  using branch_map_t = std::unordered_map<branch_id_t, branch_t*,
    branch_id_hasher__<branch_int_t, dimension>>;

  branch_map_t branch_map_;

  std::array<element_t, dimension * 2> bounds_;
  
  size_t max_depth_;
}; // ntree

template<typename T, size_t D>
class ntree_entity{
public:
  using branch_id_t = branch_id<T, D>;

  ntree_entity()
  : branch_id_(branch_id_t::null()){}

  void set_branch_id_(branch_id_t bid){
    branch_id_ = bid;
  }

  branch_id_t branch_id() const{
    return branch_id_;
  }

private:
  branch_id_t branch_id_;
};

template<typename T, size_t D>
class ntree_branch{
public:

  using branch_int_t = T;

  static const size_t dimension = D;

  using branch_id_t = branch_id<T, D>;

  ntree_branch()
  : action_(action::none){}

  void set_id_(branch_id_t id){
    id_ = id;
  }

  branch_id_t id() const{
    return id_;
  }

  void refine(){
    action_ = action::refine;
  }

  void coarsen(){
    action_ = action::coarsen;
  }

  void reset(){
    action_ = action::none;
  }

  action requested_action(){
    return action_;
  }

private:
  branch_id_t id_;
  action action_ : 2;
};

class ntree_policy{
public:
  using tree_t = ntree<ntree_policy>;

  using branch_int_t = uint64_t;

  static const size_t dimension = 2;

  using element_t = double;

  using point_t = coordinates<element_t, dimension>;

  class entity : public ntree_entity<branch_int_t, dimension>{
  public:
    entity(const point_t& p)
    : coordinates_(p){}

    const point_t& coordinates() const{
      return coordinates_;
    }

    private:
      point_t coordinates_;
  };

  using entity_t = entity;

  class branch : public ntree_branch<branch_int_t, dimension>{
  public:
    branch(){}

    void insert(entity_t* ent){
      ents_.push_back(ent);
      
      if(ents_.size() > 1){
        refine();
      }
    }

    void remove(entity_t* ent){
      auto itr = std::find(ents_.begin(), ents_.end(), ent);
      ents_.erase(itr);
      
      if(ents_.empty()){
        coarsen();
      }
    }

    auto begin(){
      return ents_.begin();
    }

    auto end(){
      return ents_.end();
    }

    void clear(){
      ents_.clear();
    }

  private:
    std::vector<entity_t*> ents_;
  };

  using branch_t = branch;
};

} // namespace ntree_dev
} // namespace flecsi