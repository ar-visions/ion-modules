#pragma once

/// composer is the abstract for managing component instancing, bindings & rendition
struct Composer {
    Interface    *ux;
    node         *root = null;
    FnRender      fn;
    Args          args;
    vec2i        *sz;
    
    vec<node *> select_at(vec2 cur, bool active = true) {
        auto inside = root->select([&](node *n) {
            rectd &bb = n->path;
            real    x = cur.x, y = cur.y;
            vec2    o = n->offset();
            vec2  rel = { x - o.x, y - o.y };
            bool   in = (x >= bb.x) && (y >= bb.y) && (x < bb.x + bb.w) && (y < bb.y + bb.h);
            return (in && (!active || !n->m.active())) ? n : null;
        });
        auto  actives = root->select([&](node *n) {
            return (active && n->m.active()) ? n : null;
        });
        auto   result = vec<node *>(inside.size() + actives.size());
        for (auto &i: inside)
              result += i;
        for (auto &i: actives)
              result += i;
        return result;
    }
    
    void event_dispatch(vec<node *> &dispatch, FnNode proxy) {
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
        vec<node *> prev_cursor;
        root->exec([&](node *n) {
            if (n->m.cursor()) {
                prev_cursor += n;
                if (n->m.cursor())
                    n->m.cursor = vec2(null);
            }
        });
        /// select all within cursor, including active
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
            Fn &fn  = (n->m.ev.cursor)();
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
    
    void resize(Window &w) {
        int test = 0;
        test++;
    }
    
    void key(Window &w, int key, int scan, int action, int mod) { /// make action an enum across the app
    }
    
    void character(Window &win, uint32_t c) {
    }
    
    bool process() {
        return root ? root->process() : true;
    }

    Composer(Interface *ux, FnRender fn, Args &args) : ux(ux), fn(fn), args(args) { }

    /// this is the update.
    bool update(node *parent, node **p_child, Element &e) {
        node *child       = *p_child;
        bool node_updated = false;
        
        ///
        if (!child) {
            child         = (*p_child = e.factory());
            child->root   = parent ? parent->root : child;
            if (!parent) {
                /// root element is initialized with the window size
                child->path = rectd { 0.0, 0.0, double(ux->sz.x), double(ux->sz.y) };
            }
            child->parent   = parent;
            child->standard_style();
            child->standard_bind();
            child->bind();
            
            for (auto &[name, member]: child->externals)   member->node = child;
            for (auto &[name, member]: child->internals)   member->node = child;
            for (auto &[name, member]: child->contextuals) member->node = child; /// call for style after all of this updating, not during
            
            node_updated = true;
        } else if (!e) {
            assert(false);
            if (child) {
                if (child->mounted)
                    child->umount();
                child->parent = null;
                delete child;
               *p_child = null;
            }
            return true;
        }
        
        /// initialize unbound check
        var d_null = nullptr;
        map<std::string, bool> unbound;
        for (auto &bind: child->binds)
            unbound[bind.id] = true;
        
        /// change members
        auto changed = vec<str>(e.binds.size());
        for (auto &bind: e.binds) {
            const str &child_mname  = bind.id;
            ///
            bool child_ctx = child->contextuals.count(child_mname) > 0;
            if (child_ctx) {
                *child->contextuals[child_mname] = bind.to;
            } else {
                const str &parent_mname = bind.to;
                ///
                bool has_ext   = parent->externals.count(parent_mname) > 0;
                bool has_int   = parent->internals.count(parent_mname) > 0;
                
                /// id is an assignment
                if (has_ext || has_int) { /// child member is never actually registered
                    const str &child_id = child->m.id;
                    const str  child_cn = child->class_name;
                    
                    /// external must be available on child
                    if (child->externals.count(bind.id) == 0)
                        console.fatal("external:{0} not found on {1}:{2}",
                                      {child_mname, child_cn, child_id});
                    auto & child_member = (Member &)*child->externals[child_mname];
                    auto &parent_member = (Member &)(has_int ? *parent->internals  [parent_mname] : /// internal has precedence
                                                               *parent->externals  [parent_mname]);
                    /// change member, if different
                    if (!(child_member == parent_member)) {
                        child_member  = parent_member;
                        changed      += bind.id;
                    }
                } else {
                    /// no external/internal member exists on parent
                    str parent_id = parent->m.id;
                    str parent_cn = parent->class_name;
                    console.fatal("{0} not found on {1}:{2}", {parent_mname, parent_cn, parent_id});
                }
            }
            if (unbound.count(bind.id))
                unbound[bind.id] = false;
        }
        
        /// unset all unbound members
        bool cycle;
        do {
            size_t index = 0;
            cycle        = false;
            for (auto &bind: child->binds) {
                if (unbound[bind.id]) {
                    changed += bind.id;
                    assert(child->externals.count(bind.id) == 1);
                    auto &member = *child->externals[bind.id];
                    member.fn->unset(member);
                    child->binds.erase(index);
                    cycle = true;
                    break;
                }
                index++;
            }
        } while (cycle);
        
        /// check if the elements are different
        if (child->elements != e.elements) {
            child->elements  = e.elements;
            node_updated = true;
        }
        
        if (changed.size())
            node_updated = true;
        auto u = map<std::string, bool>();
        for (auto &[id, nn]: child->mounts)
            u[id] = true;
        
        /// render uses elements data to produce the instances
        if (!child->mounted) {
             child->mount();
             child->mounted = true;
        }
        
        /// render child, and call update with those elements
        Element ee = child->render();
        if (ee.ref) { /// hunt these cases down.
            assert(ee.ref == child);
            node    *child = ee.ref;
            size_t      sz = child->elements.size();
            auto        um = map<std::string, bool>(sz);
            for (size_t  i = 0; i < child->elements.size(); i++) {
                auto  &eee = child->elements[i];
                std::string &id = eee.id();
                update(child, &child->mounts[id], eee);
                u[id] = false;
            }
        } else if (ee.factory) {
            std::string &id = ee.id();
            update(child, &child->mounts[id], ee);
            u[id] = false;
        }
        
        /// perform unmounting
        Element e_null = nullptr;
        for (auto &[id, unmount]: u)
            if (unmount) {
                std::string i = id;
                update(child, &child->mounts[id], e_null);
                node_updated = true;
            }
        if (node_updated)
            child->changed(changed);
        
        if (!parent && child) {
            // set style if conditions are met:
            // -> node added, removed
            // -> state change at all in the entire tree
            child->exec([&](node *n) {
                for (auto &[name, member]: n->externals) {
                    member->fn->compute_style(*member); /// no internals, they read only makes them ideal for state match
                }
            });
        }
        return node_updated;
    }
    
    void render() {
        /// ----------------------------------------
        Element  e = fn();
        update(null, &root, e);
        rectd rect = { real(0), real(0), real(sz->x), real(sz->y) };
        root->path = rect;
        /// ----------------------------------------
        root->select([&](node *n) {
            if (n != root) {
                Region &region = n->m.region;
                n->path = region(n, n->parent);
            }
            return null;
        });
        ux->draw(root);
        /// ----------------------------------------
    }
    
    vec<node *> query(std::function<node *(node *)> fn) {
        return root->select(fn);
    }

    void input(str v) {
        if (root) {
            node *f = root ->focused();
            (f  ? f : root)->input(v);
        }
    }
};
