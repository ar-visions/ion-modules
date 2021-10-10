#pragma once
#include <ux/ux.hpp>

struct Button:node {
    declare(Button);
    struct Props:IProps {
        str bind;
    } props;
};
