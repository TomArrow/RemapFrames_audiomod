/*****************************************************************************

        Calc.cpp
        Author: Laurent de Soras, 2013

*Tab=4***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "Calc.h"

#include <algorithm>

#include <cassert>
#include <cctype>
#include <cmath>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Calc::Calc ()
:   _node_list ()
,   _input_arr ()
{
    _op_info [Op_LIT   ] = OpInfo (0, 0);
    _op_info [Op_VAR   ] = OpInfo (0, 0);
    _op_info [Op_NEG   ] = OpInfo (1, "neg");
    _op_info [Op_NOT   ] = OpInfo (1, "!");
    _op_info [Op_ABS   ] = OpInfo (1, "abs");
    _op_info [Op_ROUND ] = OpInfo (1, "round");
    _op_info [Op_FLOOR ] = OpInfo (1, "floor");
    _op_info [Op_CEIL  ] = OpInfo (1, "ceil");
    _op_info [Op_ADD   ] = OpInfo (2, "+");
    _op_info [Op_SUB   ] = OpInfo (2, "-");
    _op_info [Op_MUL   ] = OpInfo (2, "*");
    _op_info [Op_DIV   ] = OpInfo (2, "/");
    _op_info [Op_MOD   ] = OpInfo (2, "mod");
    _op_info [Op_MIN   ] = OpInfo (2, "min");
    _op_info [Op_MAX   ] = OpInfo (2, "max");
    _op_info [OP_EQ    ] = OpInfo (2, "==");
    _op_info [OP_NE    ] = OpInfo (2, "!=");
    _op_info [OP_GT    ] = OpInfo (2, ">");
    _op_info [OP_GE    ] = OpInfo (2, ">=");
    _op_info [OP_LT    ] = OpInfo (2, "<");
    _op_info [OP_LE    ] = OpInfo (2, "<=");
    _op_info [OP_BAND  ] = OpInfo (2, "&&");
    _op_info [OP_BOR   ] = OpInfo (2, "||");
    _op_info [OP_BXOR  ] = OpInfo (2, "^^");
    _op_info [Op_CLIP  ] = OpInfo (3, "clip");
    _op_info [Op_IFELSE] = OpInfo (3, "\?");
}



// Each character of var_list is a single-letter variable.
int	Calc::parse (const std::string &expr, const std::string &var_list)
{
    assert (&expr != 0);
    assert (&var_list != 0);

    const int       nbr_var = int (var_list.length ());
    _input_arr.resize (nbr_var);
    for (int v = 0; v < nbr_var; ++v)
    {
        assert (isalpha (var_list [v]));

        NodeSPtr        var_sptr (new CalcNode);
        var_sptr->_op = Op_VAR;
        var_sptr->_content._index = v;

        _input_arr [v] = var_sptr;
    }

    TokList         tok_list;
    tokenize (tok_list, expr);

    _node_list.clear ();
    NodeSPtr        root_sptr;
    TokList::size_type  pos = tok_list.size ();
    int             ret_val = parse_rec (tok_list, var_list, root_sptr, pos);
    assert (pos >= 0);

    if (ret_val == 0 && pos > 0)
    {
        ret_val = -1;
    }
    if (root_sptr.get () == 0 || root_sptr->_op == Op_INVALID)
    {
        ret_val = -1;
    }

    if (ret_val == 0)
    {
        _node_list.push_back (root_sptr);
    }

    return (ret_val);
}



double	Calc::eval (const double in_arr []) const
{
    assert (! _node_list.empty ());

    const double    result = eval_node_rec (*(_node_list.back ()), in_arr);

    return (result);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Calc::OpInfo::OpInfo ()
:   _nbr_arg (-1)
,   _txt_0 (0)
{
    // Nothing
}



Calc::OpInfo::OpInfo (int nbr_arg, const char *txt_0)
:   _nbr_arg (nbr_arg)
,   _txt_0 (txt_0)
{
	assert (nbr_arg >= 0);
	assert (nbr_arg > 0 || txt_0 == 0);
	assert (txt_0 != 0 || nbr_arg == 0);
}



// Could be replaced with a better parser (natural expression writing) to
// construct the Abstract Syntax Tree.
int	Calc::parse_rec (const TokList &tok_list, const std::string &var_list, NodeSPtr &node_sptr, TokList::size_type &pos)
{
    assert (&tok_list != 0);
    assert (&var_list != 0);
    assert (&node_sptr != 0);
    assert (pos >= 0);
    assert (pos <= tok_list.size ());

    int             ret_val = 0;

    if (pos <= 0)
    {
        ret_val = -1;
    }
    else
    {
        -- pos;
        const Tok &     tok = tok_list [pos];
        bool            recog_flag = false;
        node_sptr = NodeSPtr (new CalcNode);

        // Litteral?
        const char *    start_0 = tok._val.c_str ();
        char *          stop_0;
        node_sptr->_content._val = strtod (start_0, &stop_0);
        if (stop_0 != start_0)
        {
            node_sptr->_op = Op_LIT;
            recog_flag = true;
        }

        // Input variable?
        if (! recog_flag && tok._val.length () == 1)
        {
            for (std::string::size_type scan_pos = 0
            ;   scan_pos < var_list.length () && ! recog_flag
            ;   ++ scan_pos)
            {
                const char      c = tolower (var_list [scan_pos]);
                if (tok._val [0] == c)
                {
                    node_sptr->_op = Op_VAR;
                    node_sptr->_content._index = int (scan_pos);
                    recog_flag = true;
                }
            }
        }

        // Operator?
        if (! recog_flag)
        {
            for (int scan_pos = 0
            ;   scan_pos < Op_NBR_ELT && ! recog_flag && ret_val == 0
            ;   ++ scan_pos)
            {
                const OpInfo &  op = _op_info [scan_pos];
                if (op._txt_0 != 0 && strcmp (tok._val.c_str (), op._txt_0) == 0)
                {
                    node_sptr->_op = static_cast <Op> (scan_pos);

                    const size_t        arg_pos = _node_list.size ();
                    _node_list.resize (arg_pos + op._nbr_arg);

                    for (int arg_cnt = op._nbr_arg - 1
                    ;   arg_cnt >= 0 && ret_val == 0
                    ;   -- arg_cnt)
                    {
                        NodeSPtr        arg_sptr;
                        ret_val = parse_rec (tok_list, var_list, arg_sptr, pos);
                        if (ret_val == 0)
                        {
                            _node_list.push_back (arg_sptr);
                            node_sptr->_content._children_ptr [arg_cnt] = arg_sptr.get ();
                        }
                    }

                    recog_flag = true;
                }
            }
        }

        if (! recog_flag)
        {
            ret_val = -1;
        }
    }

    return (ret_val);
}



double	Calc::eval_node_rec (CalcNode &node, const double in_arr []) const
{
    assert (&node != 0);
    assert (in_arr != 0);

    double          val = 0;
    double          tmp [3];
    for (int i = 0; i < _op_info [node._op]._nbr_arg; ++i)
    {
        tmp [i] = eval_node_rec (*(node._content._children_ptr [i]), in_arr);
    }

    switch (node._op)
    {
    case Op_LIT:
        val = node._content._val;
        break;

    case Op_VAR:
        assert (node._content._index >= 0);
        assert (node._content._index < int (_input_arr.size ()));
        val = in_arr [node._content._index];
        break;

    case Op_NEG:    val = -tmp [0];                                     break;
    case Op_NOT:    val = (tmp [0] == 0) ? 1 : 0;                       break;
    case Op_ABS:    val = fabs (tmp [0]);                               break;
    case Op_ROUND:  val = floor (tmp [0] + 0.5);                        break;
    case Op_FLOOR:  val = floor (tmp [0]);                              break;
    case Op_CEIL:   val = ceil (tmp [0]);                               break;
    case Op_ADD:    val = tmp [0] + tmp [1];                            break;
    case Op_SUB:    val = tmp [0] - tmp [1];                            break;
    case Op_MUL:    val = tmp [0] * tmp [1];                            break;
    case Op_DIV:    val = tmp [0] / tmp [1];                            break;
    case Op_MOD:    val = fmod (tmp [0], tmp [1]);                      break;
    case Op_MIN:    val = std::min (tmp [0], tmp [1]);                  break;
    case Op_MAX:    val = std::max (tmp [0], tmp [1]);                  break;
    case OP_EQ:     val = (tmp [0] == tmp [1]);                         break;
    case OP_NE:     val = (tmp [0] != tmp [1]);                         break;
    case OP_GT:     val = (tmp [0] >  tmp [1]);                         break;
    case OP_GE:     val = (tmp [0] >= tmp [1]);                         break;
    case OP_LT:     val = (tmp [0] <  tmp [1]);                         break;
    case OP_LE:     val = (tmp [0] <= tmp [1]);                         break;
    case OP_BAND:   val = (tmp [0] != 0 && tmp [1] != 0) ? 1 : 0;       break;
    case OP_BOR:    val = (tmp [0] != 0 || tmp [1] != 0) ? 1 : 0;       break;
    case OP_BXOR:   val = ((tmp [0] != 0) ^ (tmp [1] != 0)) ? 1 : 0;    break;
    case Op_CLIP:   val = std::min (std::max (tmp [0], tmp [1]), tmp [2]); break;
    case Op_IFELSE: val = (tmp [0] != 0) ? tmp [1] : tmp [2];           break;

    default:
        assert (false);
        break;
    }

	return (val);
}



// Converts alphanumeric tokens to lower case.
void	Calc::tokenize (TokList &tok_list, const std::string &expr)
{
    assert (&tok_list != 0);
    assert (&expr != 0);

    tok_list.clear ();

    enum State
    {
        State_FIND = 0,
        State_TOKEN,

        State_NBR_ELT
    };

    State           state = State_FIND;
    Tok             tok;
    std::string::size_type  pos = 0;

    while (pos < expr.length ())
    {
        const bool      s_flag = isspace (expr [pos]);
        if (state == State_TOKEN)
        {
            if (! s_flag)
            {
                tok._val += tolower (expr [pos]);
                ++ pos;
            }
            else
            {
                tok_list.push_back (tok);
                state = State_FIND;
            }
        }

        // State_FIND
        else
        {
            // Simple whitespace
            if (s_flag)
            {
                ++ pos;
            }

            // Token
            else
            {
                tok._pos = pos;
                tok._val.clear ();
                state = State_TOKEN;
            }
        }
    }

    if (state == State_TOKEN)
    {
        tok_list.push_back (tok);
    }
}



void	Calc::trim_wspaces (std::string &s)
{
    assert (&s != 0);

    std::string::size_type  pos_beg = 0;
    while (pos_beg < s.size () && isspace (s [pos_beg]))
    {
        ++ pos_beg;
    }

    std::string::size_type  pos_end = s.size ();
    while (pos_end > pos_beg && isspace (s [pos_end - 1]))
    {
        -- pos_end;
    }

    s = s.substr (pos_beg, pos_end - pos_beg);
}



// Initialised in the constructor
Calc::OpInfo    Calc::_op_info [Op_NBR_ELT];



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
