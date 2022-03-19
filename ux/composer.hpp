#pragma once

#if !defined(NDEBUG)
#include <dx/watch.hpp>
#endif

/// composer is the abstract for managing component instancing, bindings & rendition
struct Composer {
    Interface    *ux;
    node         *root = null;
    FnRender      fn;
    Map           args;
    vec2i        *sz;
    
    array<node *> select_at(vec2 cur, bool active = true) {
        auto     inside = root->select([&](node *n) {
            real      x = cur.x, y = cur.y;
            vec2      o = n->offset();
            vec2    rel = cur + o;
            bool     in = n->paths.fill.contains(rel);
            n->m.cursor = in ? vec2(x, y) : vec2(null);
            return (in && (!active || !n->m.active())) ? n : null;
        });
        auto    actives = root->select([&](node *n) {
            return (active && n->m.active()) ? n : null;
        });
        auto   result = array<node *>(size_t(inside.size()) + size_t(actives.size()));
        for (auto &i: inside)
            result += i;
        for (auto &i: actives)
            result += i;
        return result;
    }
    
    void event_dispatch(array<node *> &dispatch, FnNode proxy) {
        var      event = var(Type::Map);
        bool using_def = true;
        Fn prevent_def = [&](var &e) { using_def = false; };
        event["prevent_default"] = prevent_def;
        for (node *n: dispatch) {
            proxy(event, n);
            if (!using_def)
                break;
        }
    }
    /// merge capture and active, call it active.
    /// i cannot believe its not javascript [/butter]
    void button(Window &w, int button, int action, int mods) {
        /// needs the cursor to know if we issue a click
        bool   down = action != 0;
        auto  nodes = select_at(w.cursor(), !down);
        auto   prev = root->select([&](node *n) {
            bool active = n->m.active();
            n->m.active = false;
            return (!down && active) ? n : null;
        });
        ///
        auto set_event = [&](Event ev) {
            ev["button"] = button;
            ev["action"] = action;
            ev["mods"]   = mods;
        };
        ///
        if (down)
            event_dispatch(nodes, FnNode([&](Event ev, node *n) {
                n->m.active = true;
                console.log("active = {0}", {n->m.active()});
                if (n->m.ev.down()) {
                    set_event(ev);
                    n->m.ev.down()(ev);
                }
            }));
        /// call up events only on previously down nodes
        event_dispatch(prev, FnNode([&](Event ev, node *n) {
            if (n->m.ev.up && !n->m.active()) {
                set_event(ev);
                n->m.ev.up()(ev);
            }
        }));
    }
    
    /// cursor always updated before button, and always the same state:cursor (glfw enforced)
    void cursor(Window &w, real x, real y) {
        /// nullify cursor values
        array<node *> prev_cursor;
 
        root->exec([&](node *n) {
            if (n->m.cursor()) { /// the cursor is referencing freed memory.  so its reference should have not been freed since its still in use.  track it.
                prev_cursor += n;
                n->m.cursor  = vec2(null);
            }
        });
        
        /// select all within cursor (can be a rect potentially as a ortho selection cursor), including active
        auto nodes = select_at(w.cursor(), true);
        event_dispatch(nodes, FnNode([&](Event ev, node *n) {
            vec2  o = n->offset();
            vec2  p = { x - o.x, y - o.y };
            ev["x"] = p.x;
            ev["y"] = p.y;
            if (prev_cursor.count(n) == 0) {
                n->m.hover = true;
                auto &fn_hover = n->m.ev.hover();
                if (fn_hover)
                    fn_hover(ev);
            }
            n->m.cursor = p;
            Fn &fn = (n->m.ev.cursor)();
            if (fn)
                fn(ev);
        }));
        
        /// notify uncursored
        for (node *n:prev_cursor)
            if (!n->m.cursor()) {
                n->m.hover = false;
                auto &fn_out = n->m.ev.out();
                if   (fn_out) {
                    var ev = var(Type::Map);
                    fn_out(ev);
                }
            }
    }
    
    void resize(Window &w) { }
    
    void key(Window &w, int key, int scan, int action, int mod) { }
    
    void character(Window &win, uint32_t c) { }
    
    bool process() { return root ? root->process() : true; }

    Composer(Interface *ux, FnRender fn, Map &args) : ux(ux), fn(fn), args(args) { }

    static void children_update(vec2i &sz, node *child, Element &e, map<string, bool> &u) {
        assert(e.ref() == child);
        auto fn_update = [&](Element &ie) {
            str    &id = ie.ident();
            u[id]      = false;
            update(sz, child, &child->mounts[id], ie);
        };
        ///
        for (auto &ie: child->elements) {
            if (ie.constructable())
                fn_update(ie);
            else for (auto &ae: ie.e->elements)
                fn_update(ae);
        }
    }

    /// allocation from factory call if null with valid Element
    static node *read_child(vec2i &sz, node *parent, node **p_child, Element &e, bool &node_updated) {
        node *child = *p_child;
        if (!child) {
            child           = (*p_child = e.factory()); /// the factory gives it  what
            child->root     = parent ? parent->root : child;
            if (!parent) {
                /// root element is initialized with the window size
                /// use routine on paths to determine if mouse is inside.
                child->paths.rect = rectd { 0.0, 0.0, double(sz.x), double(sz.y) };
            }
            child->parent   = parent;
            child->binds    = e.binds();
            child->elements = e.elements();
            child->id       = e.ident();
            child->standard_bind();
            child->bind();
            ///
            node_updated = true;
        } else if (!e) {
            assert(false);
            if (child) {
                if (child->mounted)
                    child->umount();
                child->parent = null;
                delete child;
               *p_child = null;
                child   = null;
            }
        }
        return child;
    }

    static array<str> update_binds(vec2i &sz, node *parent, node *child, Element &e, bool &node_updated) {
        /// initialize unbound check
        /// if there is a state update, it should set all as changed.
        bool is_update = false;//child->flags & node::StateUpdate;
        map<string, bool> unbound;
        for (auto &bind: child->binds)
            unbound[bind.id] = true;

        /// change members
        auto changed = array<str>(e.e->binds.size());

        for (Bind &bind: e.e->binds) {
            /// the ref is just a member ref (it could potentially be grabbed backwards but only nutters would)
            /// so the receiver child gets its member set to this ref, deref'd
            if (bind.shared) {
                console.assertion(child->externals.count(bind.id) == 1, "{0} not found on {1}", { bind.id, str(child->class_name) });
                Member &child_member = (Member &)*child->externals[bind.id]; // child_member == null
                /// could be nice to have direct set access through generic shared interface
                if (is_update || !(child_member.shared == bind.shared)) {
                    child_member.lambdas->type_set(child_member, bind.shared);
                    changed += bind.id;
                }
            } else
                assert(false);
            
            if (unbound.count(bind.id))
                unbound[bind.id] = false;
        }
        
        /// unset all unbound members
        bool set_binds = false;
        for (auto &bind: child->binds) {
            if (unbound[bind.id]) {
                changed += bind.id;
                assert(child->externals.count(bind.id) == 1);
                auto &member = *child->externals[bind.id];
                member.lambdas->unset(member);
                set_binds = true;
                break;
            }
        }
        
        /// check if the elements are different
        if (set_binds || child->elements != e.e->elements) {
            child->binds    = e.e->binds;
            child->elements = e.e->elements;
            node_updated    = true;
        }
        
        child->flags &= ~node::StateUpdate;
        return changed;
    }
    
    /// update_node at the point of parent node instance and child(element)
    /// it needs to be reduced further, to lose the p_child 
    static bool update(vec2i &sz, node *parent, node **p_child, Element &e) {
        bool node_updated = false;
        node *child       = read_child(sz, parent, p_child, e, node_updated);
        if (!child)
            return true;
        
        /// style reload, the watcher is going to knock out the style handles when it changes.
        if (!child->style)
             child->style = Style::for_class(child->class_name);
        
        /// update binds, get list of changed members on the child
        auto changed = update_binds(sz, parent, child, e, node_updated);
        
        if (!child->mounted) {
             child->mount();
             child->mounted = true;
        }
        
        /// call changed before a render
        if (node_updated)
            child->changed(changed);
        
        /// render child
        Element ee = child->render();

        /// flag for removal
        auto u = map<string, bool>();
        for (auto &[id, nn]: child->mounts)
            u[id] = true;
        
        /// and call update with those elements (rendered elements: ee)
        if (ee.ref()) { /// end-point render (end of indirection, that is. time to do things.. call again, yeah thats right)
            children_update(sz, child, ee, u);
        } else if (ee.constructable()) {
            str &id = ee.ident();
            update(sz, child, &child->mounts[id], ee);
            u[id] = false;
        } else {
            /// it can definitely be an element array from filter or map call, so we're on notice here.
            //assert(false);
        }
        
        /// perform unmounting
        for (auto &[id, unmount]: u)
            if (unmount) {
                static Element e_null;
                update(sz, child, &child->mounts[id], e_null);
                node_updated = true;
            }
        
        /// perform this just for root (should be done by the ux, after this call; after-all we may not even host style facility in the app)
        if (!parent && child)
            child->exec([](node *n) {
                for (auto &[name, member]: n->externals)
                    member->lambdas->compute_style(*member);
            });
        
        return node_updated;
    }
    
    void render() {
        Element  e = fn();
        update(ux->sz, null, &root, e);
        
#if !defined(NDEBUG)
        /// composition force-update (must also signal a cache invalidation) on Watch, when debugging.
        /// this applies to style, images, vectors, objs, frags and verts
        /// it will also apply to shaders and 3D objects
        static auto css_watch = Watch::spawn(
              {"style","images","models","shaders"}, {".css",".png",".svg",".obj",".frag",".vert"}, null,
            [root=root] (bool init, array<PathOp> &paths) {
            ///
            if (!init) {
                Style::unload();
                root->exec([](node *n) {
                    n->style  = null;
                    n->flags |= node::StateUpdate;
                    for (auto &[k,m]:n->externals)
                        m->style_value_set(null, null);
                });
            }
        });
#endif
        rectd   rect = { real(0), real(0), real(sz->x), real(sz->y) };
        bool resized = root->paths.rect != rect;
        if (resized)
            root->paths.rect = rect;
        ///
        root->exec([&](node *n) -> node* {
            /// update regionized rects on nodes
            int                   index      = 0;
            node                 *rn         = n->parent ? n->parent : root;
            array<node::RegionOp> region_ops = {
                { n->m.region,            rn, rn->paths.rect, n->paths.rect },
                { n->m.fill.image_region, n,   n->paths.rect, n->image_rect },
                { n->m.text.region,       n,   n->paths.rect, n->text_rect  }
            };
            
            /// redo this, its writing to the one its then reading from.. come friend.
            for (node::RegionOp &reg_op:region_ops)
                reg_op(n, index++);
            ///
            rectd &r = n->paths.rect;
            real  TL = n->m.radius(node::Members::Radius::TL);
            real  TR = n->m.radius(node::Members::Radius::TR);
            real  BR = n->m.radius(node::Members::Radius::BR);
            real  BL = n->m.radius(node::Members::Radius::BL);
            real  fo = n->m.fill.offset;
            real  bo = n->m.border.offset;
            real  co = n->m.child.offset;
            real  sg = -1234 + (TL) + (TR * 100) + (BR * 1000) + (BL * 10000) + (fo * 50) + (bo * 500) + (co * 5000);
                  sg += (1     * 12)    * r.x;
                  sg += (10    * 321)   * r.y;
                  sg += (1000  * 1234)  * r.w;
                  sg += (10000 * 54321) * r.h;
            ///
            if (resized || n->paths.last_sg != sg) {
                n->paths.last_sg  = sg;
                vec2 v00 = r.xy();
                vec2 v10 = v00 + vec2 { r.w,   0 };
                vec2 v11 = v00 + vec2 { r.w, r.h };
                vec2 v01 = v00 + vec2 {   0, r.h };
                vec4 x00 = vec4(v00.x, v00.y, TL, TL);
                vec4 x10 = vec4(v10.x, v10.y, TR, TR);
                vec4 x11 = vec4(v11.x, v11.y, BR, BR);
                vec4 x01 = vec4(v01.x, v01.y, BL, BL);
                /// --------------------------------
                n->paths.fill   = Stroke();
                n->paths.fill.rect_v4(x00, x10, x11, x01);
                n->paths.border = n->paths.fill;
                n->paths.child  = n->paths.fill;
                n->paths.fill.  set_offset(fo);
                n->paths.border.set_offset(bo);
                n->paths.child. set_offset(co);
            }
            return null;
        });
        ///
        ux->draw(root);
    }
    
    array<node *> query(std::function<node *(node *)> fn) {
        return root->select(fn);
    }

    void input(str v) {
        if (root) {
            node *f = root ->focused();
            (f  ? f : root)->input(v);
        }
    }
};
