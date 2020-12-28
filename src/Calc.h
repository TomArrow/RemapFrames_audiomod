/*****************************************************************************

        Calc.h
        Author: Laurent de Soras, 2013

*Tab=4***********************************************************************/



#pragma once
#if ! defined (Calc_HEADER_INCLUDED)
#define    Calc_HEADER_INCLUDED

#if defined (_MSC_VER)
    #pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "SharedPtr.h"

#include <vector>



class Calc
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

                    Calc ();
    virtual         ~Calc () {}

    int             parse (const std::string &expr, const std::string &var_list);
    double          eval (const double in_arr []) const;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

    enum Op
    {
        Op_INVALID = -1,

        Op_LIT = 0,
        Op_VAR,
        Op_NEG,
        Op_NOT,
        Op_ABS,
        Op_ROUND,
        Op_FLOOR,
        Op_CEIL,
        Op_ADD,
        Op_SUB,
        Op_MUL,
        Op_DIV,
        Op_MOD,
        Op_MIN,
        Op_MAX,
        OP_EQ,
        OP_NE,
        OP_GT,
        OP_GE,
        OP_LT,
        OP_LE,
        OP_BAND,
        OP_BOR,
        OP_BXOR,
        Op_CLIP,
        Op_IFELSE,

        Op_NBR_ELT
    };

    class CalcNode
    {
    public:
                        CalcNode () : _op (Op_INVALID) { }
        Op              _op;
        union
        {
            double          _val;               // Only for Op_LIT
            int             _index;             // Only for Op_VAR
            CalcNode *      _children_ptr [3];
        }               _content;
    };

    typedef SharedPtr <CalcNode> NodeSPtr;

    class Tok
    {
    public:
        std::string     _val;
        std::string::size_type
                        _pos;
    };

    class OpInfo
    {
    public:
        explicit        OpInfo ();
        explicit        OpInfo (int nbr_arg, const char *txt_0);
        int             _nbr_arg;   // 0 if not an operator.
        const char *    _txt_0;     // Operator name. Lower case. 0 if it isn't an operator.
    };

    typedef std::vector <Tok> TokList;

    int             parse_rec (const TokList &tok_list, const std::string &var_list, NodeSPtr &node_sptr, TokList::size_type &pos);
    double          eval_node_rec (CalcNode &node, const double in_arr []) const;

    static void     tokenize (TokList &tok_list, const std::string &expr);
    static void     trim_wspaces (std::string &s);

    std::vector <NodeSPtr>
                    _node_list;
    std::vector <NodeSPtr>
                    _input_arr;

    static OpInfo   _op_info [Op_NBR_ELT];     // For operators only



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

                    Calc (const Calc &other);
    Calc &          operator = (const Calc &other);
    bool            operator == (const Calc &other) const;
    bool            operator != (const Calc &other) const;

};    // class Calc



//#include "Calc.hpp"



#endif    // Calc_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
