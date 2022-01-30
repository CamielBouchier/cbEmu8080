
require 'cbVtParserTables'

class String
    def pad(len)
        self << (" " * (len - self.length))
    end
end

File.open("cbVtParserTables.h", "w") { |f|
    f.puts
    f.puts '#include <stdint.h>'
    f.puts
    $states_in_order.each_with_index { |state, i|
        f.puts "static const int VTPARSE_STATE_#{state.to_s.upcase} = #{i+1};"
    }
    f.puts
    $actions_in_order.each_with_index { |action, i|
        f.puts "static const int VTPARSE_ACTION_#{action.to_s.upcase} = #{i+1};"
    }
    f.puts
    f.puts "extern uint8_t STATE_TABLE[#{$states_in_order.length}][256];"
    f.puts "extern int ENTRY_ACTIONS[#{$states_in_order.length}];"
    f.puts "extern int EXIT_ACTIONS[#{$states_in_order.length}];"
    f.puts "extern const char *ACTION_NAMES[#{$actions_in_order.length+1}];"
    f.puts "extern const char *STATE_NAMES[#{$states_in_order.length+1}];"
    f.puts
}

puts "Wrote cbVtParserTables.h"

File.open("cbVtParserTables.cpp", "w") { |f|
    f.puts
    f.puts '#include "cbVtParserTables.h"'
    f.puts
    f.puts "const char *ACTION_NAMES[] = {"
    f.puts "   \"<no action>\","
    $actions_in_order.each { |action|
        f.puts "   \"#{action.to_s.upcase}\","
    }
    f.puts "};"
    f.puts
    f.puts "const char *STATE_NAMES[] = {"
    f.puts "   \"<no state>\","
    $states_in_order.each { |state|
        f.puts "   \"#{state.to_s}\","
    }
    f.puts "};"
    f.puts
    f.puts "uint8_t STATE_TABLE[#{$states_in_order.length}][256] = {"
    $states_in_order.each_with_index { |state, i|
        f.puts "  {  /* VTPARSE_STATE_#{state.to_s.upcase} = #{i} */"
        $state_tables[state].each_with_index { |state_change, i|
            if not state_change
                f.puts "    0,"
            else
                (action,) = state_change.find_all { |s| s.kind_of?(Symbol) }
                (state,)  = state_change.find_all { |s| s.kind_of?(StateTransition) }
                action_str = action ? "VTPARSE_ACTION_#{action.to_s.upcase}" : "0"
                state_str =  state ? "VTPARSE_STATE_#{state.to_state.to_s}" : "0"
                f.puts "/*#{i.to_s.pad(3)}*/  #{action_str.pad(33)} | (#{state_str.pad(33)} << 4),"
            end
        }
        f.puts "  },"
    }

    f.puts "};"
    f.puts
    f.puts "int ENTRY_ACTIONS[] = {"
    $states_in_order.each { |state|
        actions = $states[state]
        if actions[:on_entry]
            f.puts "   VTPARSE_ACTION_#{actions[:on_entry].to_s.upcase}, /* #{state} */"
        else
            f.puts "   0  /* none for #{state} */,"
        end
    }
    f.puts "};"
    f.puts
    f.puts "int EXIT_ACTIONS[] = {"
    $states_in_order.each { |state|
        actions = $states[state]
        if actions[:on_exit]
            f.puts "   VTPARSE_ACTION_#{actions[:on_exit].to_s.upcase}, /* #{state} */"
        else
            f.puts "   0  /* none for #{state} */,"
        end
    }
    f.puts "};"
    f.puts
}

puts "Wrote cbVtParserTables.cpp"
