/************************************************************************/
/*                                                                      */
/*                 Copyright 2011 by Ullrich Koethe                     */
/*                                                                      */
/*    This file is part of the VIGRA computer vision library.           */
/*    The VIGRA Website is                                              */
/*        http://hci.iwr.uni-heidelberg.de/vigra/                       */
/*    Please direct questions, bug reports, and contributions to        */
/*        ullrich.koethe@iwr.uni-heidelberg.de    or                    */
/*        vigra@informatik.uni-hamburg.de                               */
/*                                                                      */
/*    Permission is hereby granted, free of charge, to any person       */
/*    obtaining a copy of this software and associated documentation    */
/*    files (the "Software"), to deal in the Software without           */
/*    restriction, including without limitation the rights to use,      */
/*    copy, modify, merge, publish, distribute, sublicense, and/or      */
/*    sell copies of the Software, and to permit persons to whom the    */
/*    Software is furnished to do so, subject to the following          */
/*    conditions:                                                       */
/*                                                                      */
/*    The above copyright notice and this permission notice shall be    */
/*    included in all copies or substantial portions of the             */
/*    Software.                                                         */
/*                                                                      */
/*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND    */
/*    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES   */
/*    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND          */
/*    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT       */
/*    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,      */
/*    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR     */
/*    OTHER DEALINGS IN THE SOFTWARE.                                   */
/*                                                                      */
/************************************************************************/


#ifndef VIGRA_ADJACENCY_LIST_GRAPH_HXX
#define VIGRA_ADJACENCY_LIST_GRAPH_HXX

/*std*/
#include <vector>
#include  <set>

/*boost*/
#include <boost/foreach.hpp>

/*vigra*/
#include <vigra/multi_array.hxx>
#include <vigra/multi_gridgraph.hxx>
#include <vigra/graphs.hxx>
#include <vigra/tinyvector.hxx>
#include <vigra/random_access_set.hxx>
#include <vigra/graph_helper/dense_map.hxx>


#include <vigra/algorithm.hxx>
#include <vigra/graph_helper/graph_item_impl.hxx>


namespace vigra{


    namespace detail_adjacency_list_graph{

        template<class G,class ITEM>
        class ItemIter
        : public boost::iterator_facade<
            ItemIter<G,ITEM>,
            const ITEM,
            boost::forward_traversal_tag
        >
        {

            typedef vigra::GraphItemHelper<G,ITEM> ItemHelper;
            typedef typename G::index_type index_type;

        public:
            ItemIter(const lemon::Invalid & iv = lemon::INVALID)
            :   graph_(NULL),
                id_(-1),
                item_(lemon::INVALID)
            {
            }

            ItemIter(const G & g)
            :   graph_(&g),
                id_(g.zeroStart() ? 0 : 1),
                item_(ItemHelper::itemFromId(*graph_,id_))
            {
                while( !isEnd()  &&  item_==lemon::INVALID ){
                    ++id_;
                    item_ = ItemHelper::itemFromId(*graph_,id_);
                }
            }

            ItemIter(const G & g,const ITEM & item)
            :   graph_(&g),
                id_(g.id(item)),
                item_(item)
            {

            }

        private:

            friend class boost::iterator_core_access;
            bool isEnd( )const{
                return graph_==NULL || id_>ItemHelper::maxItemId(*graph_);
            }
            bool isBegin( )const{
                return graph_!=NULL && (graph_->zeroStart() && id_ == 0 || !graph_->zeroStart() && id_ == 1);
            }

            bool equal(const ItemIter & other) const{
                return   (isEnd() && other.isEnd() ) || (isEnd()==other.isEnd() && (id_ == other.id_) );
            }

            void increment(){
                ++id_;
                item_ = ItemHelper::itemFromId(*graph_,id_);
                while( !isEnd()  &&  item_==lemon::INVALID ){
                    ++id_;
                    item_ = ItemHelper::itemFromId(*graph_,id_);
                }
            }
            const ITEM & dereference()const{
                return item_;
            }
            const G * graph_;
            index_type id_;
            ITEM item_;
        };



        template<class GRAPH>
        class ArcIt
        : public boost::iterator_facade<
            ArcIt<GRAPH>,
            const typename GRAPH::Arc,
            boost::forward_traversal_tag
        >
        {
        public:
            typedef GRAPH Graph;
            typedef typename  Graph::Arc Arc;
            typedef typename  Graph::Edge Edge;
            typedef typename  Graph::EdgeIt EdgeIt;
            ArcIt(const lemon::Invalid invalid = lemon::INVALID )
            :   graph_(NULL),
                pos_(),
                inFirstHalf_(false),
                veryEnd_(true),
                arc_(){
            }
            ArcIt(const GRAPH & g )
            :   graph_(&g),
                pos_(g),
                inFirstHalf_(true),
                veryEnd_( g.edgeNum()==0 ? true : false),
                arc_(){
            }

            ArcIt(const GRAPH & g , const Arc & arc )
            :   graph_(&g),
                pos_(g,arc.edgeId()),
                inFirstHalf_(g.id(arc)<=g.maxEdgeId()),
                veryEnd_(false),
                arc_(){
            }
        private:

            bool isEnd()const{
                return veryEnd_ || graph_==NULL;
            }

            bool isBegin()const{
                return graph_!=NULL &&  veryEnd_==false && pos_ == EdgeIt(*graph_);         
            }


            friend class boost::iterator_core_access;

            void increment() {
                if(inFirstHalf_){
                    ++pos_;
                    if(pos_ == lemon::INVALID  ) {
                        pos_ = EdgeIt(*graph_);
                        inFirstHalf_=false;
                    }
                    return;
                }
                else{
                    ++pos_;
                    if(pos_ == lemon::INVALID){
                        veryEnd_=true;
                    }
                    return;
                }
            
               
            }
            bool equal(ArcIt const& other) const{
                return (
                    (
                        isEnd()==other.isEnd()                  &&
                        inFirstHalf_==other.inFirstHalf_ 
                    ) &&
                    (isEnd() || graph_==NULL || pos_==other.pos_ )
                    );
                    
            }

            const Arc & dereference() const { 
                //std::cout<<graph_->id(*pos_)<<"\n";
                arc_ = graph_->direct(*pos_,inFirstHalf_);
                return arc_;
            }


            const GRAPH * graph_;
            EdgeIt pos_;
            bool inFirstHalf_;
            bool veryEnd_;
            mutable Arc arc_;
        };

    }


    
    class AdjacencyListGraph
    {
        
    public:
        // public typdedfs
        typedef Int64                                                     index_type;
    private:
        // private typedes which are needed for defining public typedes
        typedef AdjacencyListGraph                                          GraphType;
        typedef detail::GenericNodeImpl<index_type,false>                   NodeStorage;
        typedef detail::GenericEdgeImpl<index_type >                      EdgeStorage;
        typedef detail::NeighborNodeFilter<GraphType>                       NnFilter;
        typedef detail::IncEdgeFilter<GraphType>                            IncFilter;
        typedef detail::IsInFilter<GraphType>                               InFlter;
        typedef detail::IsOutFilter<GraphType>                              OutFilter;
    public:
        // LEMON API TYPEDEFS (and a few more(NeighborNodeIt))
        typedef detail::GenericNode<index_type>                           Node;
        typedef detail::GenericEdge<index_type>                           Edge;
        typedef detail::GenericArc<index_type>                            Arc;
        typedef detail_adjacency_list_graph::ItemIter<GraphType,Edge>    EdgeIt;
        typedef detail_adjacency_list_graph::ItemIter<GraphType,Node>    NodeIt; 
        typedef detail_adjacency_list_graph::ArcIt<GraphType>            ArcIt;
        
        typedef detail::GenericIncEdgeIt<GraphType,NodeStorage,IncFilter >  IncEdgeIt;
        typedef detail::GenericIncEdgeIt<GraphType,NodeStorage,InFlter   >  InArcIt;
        typedef detail::GenericIncEdgeIt<GraphType,NodeStorage,OutFilter >  OutArcIt;
        // custom but lemonsih
        typedef detail::GenericIncEdgeIt<GraphType,NodeStorage,NnFilter  >  NeighborNodeIt;

        // BOOST GRAPH API TYPEDEFS
        // - categories (not complete yet)
        typedef boost::directed_tag     directed_category;
        // iterators
        typedef NeighborNodeIt          adjacency_iterator;
        typedef EdgeIt                  edge_iterator;
        typedef NodeIt                  vertex_iterator;
        typedef IncEdgeIt               in_edge_iterator;
        typedef IncEdgeIt               out_edge_iterator;

        // size types
        typedef size_t                  degree_size_type;
        typedef size_t                  edge_size_type;
        typedef size_t                  vertex_size_type;
        // item descriptors
        typedef Edge edge_descriptor;
        typedef Node vertex_descriptor;



        template<class T>
        struct EdgeMap : DenseEdgeReferenceMap<GraphType,T> {
            EdgeMap(): DenseEdgeReferenceMap<GraphType,T>(){
            }
            EdgeMap(const GraphType & g)
            : DenseEdgeReferenceMap<GraphType,T>(g){
            }
        };

        template<class T>
        struct NodeMap : DenseNodeReferenceMap<GraphType,T> {
            NodeMap(): DenseNodeReferenceMap<GraphType,T>(){
            }
            NodeMap(const GraphType & g)
            : DenseNodeReferenceMap<GraphType,T>(g){
            }
        };

        template<class T>
        struct ArcMap : DenseArcReferenceMap<GraphType,T> {
            ArcMap(): DenseArcReferenceMap<GraphType,T>(){
            }
            ArcMap(const GraphType & g)
            : DenseArcReferenceMap<GraphType,T>(g){
            }
        };



    // public member functions
    public:
        // todo...refactor this crap...always start add zero
        // (if one wants to start at  1 just just g.addNode(1).. and so on)
        AdjacencyListGraph(const size_t nodes=0,const size_t edges=0,const bool zeroStart=false);

        index_type edgeNum()const;
        index_type nodeNum()const;
        index_type arcNum()const;

        index_type maxEdgeId()const;
        index_type maxNodeId()const;
        index_type maxArcId()const;

        Arc direct(const Edge & edge,const bool forward)const;
        Arc direct(const Edge & edge,const Node & node)const;
        bool direction(const Arc & arc)const;

        Node u(const Edge & edge)const;
        Node v(const Edge & edge)const;
        Node source(const Arc & arc)const;
        Node target(const Arc & arc)const;
        Node oppositeNode(Node const &n, const Edge &e) const;

        Node baseNode(const IncEdgeIt & iter)const;
        Node baseNode(const OutArcIt & iter)const;

        Node runningNode(const IncEdgeIt & iter)const;
        Node runningNode(const OutArcIt & iter)const;


        // ids 
        index_type id(const Node & node)const;
        index_type id(const Edge & edge)const;
        index_type id(const Arc  & arc )const;

        // get edge / node from id
        Edge edgeFromId(const index_type id)const;
        Node nodeFromId(const index_type id)const;
        Arc  arcFromId(const index_type id)const;


        // find edge
        Edge findEdge(const Node & a,const Node & b)const;
        Arc  findArc(const Node & u,const Node & v)const;


        Node addNode();
        Node addNode(const index_type id);
        Edge addEdge(const Node & u , const Node & v);
        Edge addEdge(const index_type u ,const index_type v);

        
        // TODO refactore  / remove this crap !
        bool zeroStart()const{
            return zeroStart_;
        }

        ////////////////////////
        // BOOST API
        /////////////////////////
        // - sizes 
        // - iterators
        vertex_iterator  get_vertex_iterator()const;
        vertex_iterator  get_vertex_end_iterator()const  ;
        edge_iterator    get_edge_iterator()const;
        edge_iterator    get_edge_end_iterator()const  ;
        degree_size_type degree(const vertex_descriptor & node)const{
            return nodeImpl(node).numberOfEdges();
        }

        static const bool is_directed = false;

    private:
        // private typedefs
        typedef std::vector<NodeStorage> NodeVector;
        typedef std::vector<EdgeStorage> EdgeVector;


        // needs acces to const nodeImpl
        template<class G,class NIMPL,class FILT>
        friend class detail::GenericIncEdgeIt;

        template<class G>
        friend class detail::NeighborNodeFilter;
        template<class G>
        friend class detail::IncEdgeFilter;
        template<class G>
        friend class detail::BackEdgeFilter;
        template<class G>
        friend class detail::IsOutFilter;
        template<class G>
        friend class detail::IsInFilter;


        friend class detail_adjacency_list_graph::ItemIter<GraphType,Node>;
        friend class detail_adjacency_list_graph::ItemIter<GraphType,Edge>;


        const NodeStorage & nodeImpl(const Node & node)const{
            return nodes_[node.id()];
        }

        NodeStorage & nodeImpl(const Node & node){
            return nodes_[node.id()];
        }





        // graph
        NodeVector nodes_;
        EdgeVector edges_;

        size_t nodeNum_;
        size_t edgeNum_;

        bool zeroStart_;
    };





    inline AdjacencyListGraph::AdjacencyListGraph(
        const size_t reserveNodes,
        const size_t reserveEdges,
        const bool   zeroStart
    )
    :   nodes_(),
        edges_(),
        nodeNum_(0),
        edgeNum_(0),
        zeroStart_(zeroStart)
    {
        nodes_.reserve(reserveNodes);
        edges_.reserve(reserveEdges);

        if(!zeroStart_){
            nodes_.push_back(NodeStorage(lemon::INVALID));
            edges_.push_back(EdgeStorage(lemon::INVALID));
        }
    }


    inline AdjacencyListGraph::Node 
    AdjacencyListGraph::addNode(){
        const index_type id = nodes_.size();
        nodes_.push_back(NodeStorage(id));
        ++nodeNum_;
        return Node(id);
    }

    inline AdjacencyListGraph::Node 
    AdjacencyListGraph::addNode(const AdjacencyListGraph::index_type id){
        if(id == nodes_.size()){
            nodes_.push_back(NodeStorage(id));
            ++nodeNum_;
            return Node(id);
        }
        else if(id<nodes_.size()){
            const Node node = nodeFromId(id);
            if(node==lemon::INVALID){
                nodes_[id]=NodeStorage(id);
                ++nodeNum_;
                return Node(id);
            }
            else{
                return node;
            }
        }
        else{
            // refactor me
            while(nodes_.size()<id){
                nodes_.push_back(NodeStorage(lemon::INVALID));
            }
            nodes_.push_back(NodeStorage(id));
            ++nodeNum_;
            return Node(id);
        }
    }

    inline AdjacencyListGraph::Edge 
    AdjacencyListGraph::addEdge(
        const AdjacencyListGraph::Node & u , 
        const AdjacencyListGraph::Node & v
    ){
        const Edge foundEdge  = findEdge(u,v);
        if(foundEdge!=lemon::INVALID){
            return foundEdge;
        }
        else if(u==lemon::INVALID || v==lemon::INVALID){
            return Edge(lemon::INVALID);
        }
        else{
            const index_type eid  = edges_.size();
            const index_type uid = u.id();
            const index_type vid = v.id();
            edges_.push_back(EdgeStorage(uid,vid,eid));
            nodeImpl(u).insert(vid,eid);
            nodeImpl(v).insert(uid,eid);
            ++edgeNum_;
            return Edge(eid);
        }   
    }

    inline AdjacencyListGraph::Edge 
    AdjacencyListGraph::addEdge(
        const AdjacencyListGraph::index_type u ,
        const AdjacencyListGraph::index_type v
    ){
        const Node uu = addNode(u);
        const Node vv = addNode(v);
        return addEdge(uu,vv);
    }

    
    
    inline AdjacencyListGraph::Arc 
    AdjacencyListGraph::direct(
        const AdjacencyListGraph::Edge & edge,
        const bool forward
    )const{
        if(edge!=lemon::INVALID){
            if(forward)
                return Arc(id(edge),id(edge));
            else
                return Arc(id(edge)+maxEdgeId()+1,id(edge));
        }
        else
            return Arc(lemon::INVALID);
    }

    
    inline AdjacencyListGraph::Arc 
    AdjacencyListGraph::direct(
        const AdjacencyListGraph::Edge & edge,
        const AdjacencyListGraph::Node & node
    )const{
        if(u(edge)==node){
            return Arc(id(edge),id(edge));
        }
        else if(v(edge)==node){
            return Arc(id(edge)+maxEdgeId()+1,id(edge));
        }
        else{
            return Arc(lemon::INVALID);
        }
    }

    
    inline bool
    AdjacencyListGraph::direction(
        const AdjacencyListGraph::Arc & arc
    )const{
        return id(arc)<=maxEdgeId();
    }

    
    inline AdjacencyListGraph::Node 
    AdjacencyListGraph::u(
        const AdjacencyListGraph::Edge & edge
    )const{
        return Node(edges_[id(edge)].u());
    }

    
    inline AdjacencyListGraph::Node
    AdjacencyListGraph::v(
        const AdjacencyListGraph::Edge & edge
    )const{
        return Node(edges_[id(edge)].v());
    }


    
    inline AdjacencyListGraph::Node 
    AdjacencyListGraph::source(
        const AdjacencyListGraph::Arc & arc
    )const{
        const index_type arcIndex  = id(arc);
        if (arcIndex > maxEdgeId() ){
            const index_type edgeIndex = arc.edgeId();
            const Edge edge = edgeFromId(edgeIndex);
            return v(edge);
        }
        else{
            const index_type edgeIndex = arcIndex;
            const Edge edge = edgeFromId(edgeIndex);
            return u(edge);
        }
    }   


    
    inline AdjacencyListGraph::Node 
    AdjacencyListGraph::target(
        const AdjacencyListGraph::Arc & arc
    )const{
        const index_type arcIndex  = id(arc);
        if (arcIndex > maxEdgeId() ){
            const index_type edgeIndex = arc.edgeId();
            const Edge edge = edgeFromId(edgeIndex);
            return u(edge);
        }
        else{
            const index_type edgeIndex = arcIndex;
            const Edge edge = edgeFromId(edgeIndex);
            return v(edge);
        }
    }

    
    inline AdjacencyListGraph::Node
    AdjacencyListGraph::oppositeNode(
        const AdjacencyListGraph::Node &n,
        const AdjacencyListGraph::Edge &e
    ) const {
        const Node uNode = u(e);
        const Node vNode = v(e);
        if(id(uNode)==id(n)){
            return vNode;
        }
        else if(id(vNode)==id(n)){
            return uNode;
        }
        else{
            return Node(-1);
        }
    }


    
    inline AdjacencyListGraph::Node 
    AdjacencyListGraph::baseNode(
        const AdjacencyListGraph::IncEdgeIt & iter
    )const{
        return u(*iter);
    }

    
    inline AdjacencyListGraph::Node 
    AdjacencyListGraph::baseNode(
        const AdjacencyListGraph::OutArcIt & iter 
    )const{
        return source(*iter);
    }


    
    inline AdjacencyListGraph::Node 
    AdjacencyListGraph::runningNode(
        const AdjacencyListGraph::IncEdgeIt & iter
    )const{
        return v(*iter);
    }

    
    inline AdjacencyListGraph::Node 
    AdjacencyListGraph::runningNode(
        const AdjacencyListGraph::OutArcIt & iter 
    )const{
        return target(*iter);
    }

    
    inline AdjacencyListGraph::index_type 
    AdjacencyListGraph::edgeNum()const{
        return edgeNum_;
    }

    
    inline AdjacencyListGraph::index_type 
    AdjacencyListGraph::nodeNum()const{
        return nodeNum_;
    }

    
    inline AdjacencyListGraph::index_type 
    AdjacencyListGraph::arcNum()const{
        return edgeNum()*2;
    }

    
    inline AdjacencyListGraph::index_type 
    AdjacencyListGraph::maxEdgeId()const{
        return edges_.back().id();
    }

    
    inline AdjacencyListGraph::index_type 
    AdjacencyListGraph::maxNodeId()const{
        return nodes_.back().id();
    }

    
    inline AdjacencyListGraph::index_type 
    AdjacencyListGraph::maxArcId()const{
        return maxEdgeId()*2+1;
    }

    // ids 
    
    inline AdjacencyListGraph::index_type 
    AdjacencyListGraph::id(
        const AdjacencyListGraph::Node & node
    )const{
        return node.id();
    }

    
    inline AdjacencyListGraph::index_type 
    AdjacencyListGraph::id(
        const AdjacencyListGraph::Edge & edge
    )const{
        return edge.id();
    }

    
    inline AdjacencyListGraph::index_type 
    AdjacencyListGraph::id(
        const AdjacencyListGraph::Arc & arc
    )const{
        return arc.id();
    }

    // get edge / node from id
    
    inline AdjacencyListGraph::Edge 
    AdjacencyListGraph::edgeFromId(
        const AdjacencyListGraph::index_type id
    )const{
        if(id<edges_.size() && edges_[id].id()!=-1)
            return Edge(edges_[id].id());
        else
            return Edge(lemon::INVALID);
    }

    
    inline AdjacencyListGraph::Node 
    AdjacencyListGraph::nodeFromId(
        const AdjacencyListGraph::index_type id
    )const{
        if(id<nodes_.size()&& nodes_[id].id()!=-1)
            return Node(nodes_[id].id());
        else
            return Node(lemon::INVALID);
    }

    
    inline AdjacencyListGraph::Arc 
    AdjacencyListGraph::arcFromId(
        const AdjacencyListGraph::index_type id
    )const{
        if(id<=maxEdgeId()){
            if(edgeFromId(id)==lemon::INVALID)
                return Arc(lemon::INVALID);
            else
                return Arc(id,id);
        }
        else{
            const index_type edgeId = id - (maxEdgeId() + 1);
            if( edgeFromId(edgeId)==lemon::INVALID)
                return Arc(lemon::INVALID);
            else
                return Arc(id,edgeId);
        }
    }

    
    inline  AdjacencyListGraph::Edge  
    AdjacencyListGraph::findEdge(
        const AdjacencyListGraph::Node & a,
        const AdjacencyListGraph::Node & b
    )const{
        if(a!=b){
            std::pair<index_type,bool> res =  nodes_[id(a)].findEdge(id(b));
            if(res.second){
                return Edge(res.first);
            }
        }
        return Edge(lemon::INVALID);
    }


    
    inline  AdjacencyListGraph::Arc  
    AdjacencyListGraph::findArc(
        const AdjacencyListGraph::Node & uNode,
        const AdjacencyListGraph::Node & vNode
    )const{
        const Edge e = findEdge(uNode,vNode);
        if(e==lemon::INVALID){
            return Arc(lemon::INVALID);
        }
        else{
            if(u(e)==uNode)
                return direct(e,true) ;
            else
                return direct(e,false) ;
        }
    }


    // iterators
    
    inline AdjacencyListGraph::vertex_iterator 
    AdjacencyListGraph::get_vertex_iterator()const{
        return NodeIt(0,nodeNum());
    }

    
    inline AdjacencyListGraph::vertex_iterator 
    AdjacencyListGraph::get_vertex_end_iterator()const{  
        return NodeIt(nodeNum(),nodeNum());
    }


    
    inline AdjacencyListGraph::edge_iterator 
    AdjacencyListGraph::get_edge_iterator()const{
        return EdgeIt(0,edgeNum());
    }

    
    inline AdjacencyListGraph::edge_iterator 
    AdjacencyListGraph::get_edge_end_iterator()const{  
        return EdgeIt(edgeNum(),edgeNum());
    }

} // end namespace vigra


// boost free functions specialized for adjacency list graph
namespace boost{

    ////////////////////////////////////
    // functions to get size of the graph
    ////////////////////////////////////
    inline vigra::AdjacencyListGraph::vertex_size_type
    num_vertices(const vigra::AdjacencyListGraph & g){
        return g.nodeNum();
    }
    inline vigra::AdjacencyListGraph::edge_size_type
    num_edges(const vigra::AdjacencyListGraph & g){
        return g.edgeNum();
    }


    ////////////////////////////////////
    // functions to get degrees of nodes
    // (degree / indegree / outdegree)
    ////////////////////////////////////
    inline vigra::AdjacencyListGraph::degree_size_type
    degree(const vigra::AdjacencyListGraph::vertex_descriptor & v , const vigra::AdjacencyListGraph & g){
        return g.degree(v);
    }
    // ??? check if this is the right impl. for undirected graphs
    inline vigra::AdjacencyListGraph::degree_size_type
    in_degree(const vigra::AdjacencyListGraph::vertex_descriptor & v , const vigra::AdjacencyListGraph & g){
        return g.degree(v);
    }
    // ??? check if this is the right impl. for undirected graphs
    inline vigra::AdjacencyListGraph::degree_size_type
    out_degree(const vigra::AdjacencyListGraph::vertex_descriptor & v , const vigra::AdjacencyListGraph & g){
        return g.degree(v);
    }


    ////////////////////////////////////
    // functions to u/v source/target
    ////////////////////////////////////
    inline vigra::AdjacencyListGraph::vertex_descriptor
    source(const vigra::AdjacencyListGraph::edge_descriptor & e , const vigra::AdjacencyListGraph & g){
        return g.u(e);
    }

    inline vigra::AdjacencyListGraph::vertex_descriptor
    target(const vigra::AdjacencyListGraph::edge_descriptor & e , const vigra::AdjacencyListGraph & g){
        return g.v(e);
    }

    ////////////////////////////////////
    // functions to get iterator pairs
    ////////////////////////////////////
    inline  std::pair< vigra::AdjacencyListGraph::vertex_iterator, vigra::AdjacencyListGraph::vertex_iterator >
    vertices(const vigra::AdjacencyListGraph & g ){
        return std::pair< vigra::AdjacencyListGraph::vertex_iterator, vigra::AdjacencyListGraph::vertex_iterator >(
            g.get_vertex_iterator(), g.get_vertex_end_iterator());
    }
    inline  std::pair< vigra::AdjacencyListGraph::edge_iterator, vigra::AdjacencyListGraph::edge_iterator >
    edges(const vigra::AdjacencyListGraph & g ){
        return std::pair< vigra::AdjacencyListGraph::edge_iterator, vigra::AdjacencyListGraph::edge_iterator >(
            g.get_edge_iterator(),g.get_edge_end_iterator());
    }


    inline  std::pair< vigra::AdjacencyListGraph::in_edge_iterator, vigra::AdjacencyListGraph::in_edge_iterator >
    in_edges(const vigra::AdjacencyListGraph::vertex_descriptor & v,  const vigra::AdjacencyListGraph & g ){
        return std::pair< vigra::AdjacencyListGraph::in_edge_iterator, vigra::AdjacencyListGraph::in_edge_iterator >(
            vigra::AdjacencyListGraph::in_edge_iterator(g,v),vigra::AdjacencyListGraph::in_edge_iterator(lemon::INVALID)
        );
    }
    inline  std::pair< vigra::AdjacencyListGraph::out_edge_iterator, vigra::AdjacencyListGraph::out_edge_iterator >
    out_edges(const vigra::AdjacencyListGraph::vertex_descriptor & v,  const vigra::AdjacencyListGraph & g ){
        return std::pair< vigra::AdjacencyListGraph::out_edge_iterator, vigra::AdjacencyListGraph::out_edge_iterator >(
            vigra::AdjacencyListGraph::out_edge_iterator(g,v),vigra::AdjacencyListGraph::out_edge_iterator(lemon::INVALID)
        );
    }

}


#endif /*VIGRA_ADJACENCY_LIST_GRAPH_HXX*/