<?php
	function expand_host_patterns($patterns) {
		$result = array();

		$host_patterns_list = preg_split('/\n/', $patterns);

		foreach ($host_patterns_list as $pattern_str)
		{
			$pattern_str = trim($pattern_str);

			if (empty($pattern_str) || preg_match('/^#/', $pattern_str))
				continue;

			$pattern_arr = split(' ', $pattern_str);
			$host_format = array_shift($pattern_arr);
			$host_pattern_args = array();

			$iterations = 1;
			foreach ($pattern_arr as $arg)
			{
				if ($arg[0] == '{')
				{
					$arg = substr($arg,1, -1);
					$lst = split(',', $arg);

					array_push($host_pattern_args, $lst);
					$iterations *= count($lst);
				}
				else if ($arg[0] == '[')
				{
					$arg = substr($arg,1, -1);
					$a = split('-', $arg);

					if (count($a) != 2 || strlen($a[0]) == 0 || strlen($a[1]) == 0)
					{
						array_push($host_pattern_args, array($arg));
						continue;
					}

					$lst = preg_grep('/[A-Za-z0-9]/',range($a[0], $a[1]));

					array_push($host_pattern_args, $lst);
					$iterations *= count($lst);
				}
				else
					array_push($host_pattern_args, array($arg));
			}

			$num_args = count($host_pattern_args);
			if (!$num_args)
			{
				array_push($result, $host_format);
				continue;
			}

			$args_max  = array();
			for ($i = 0; $i < $num_args; $i++)
				$args_max[$i] = (count($host_pattern_args[$i]) - 1);
			$args_cur  = array_fill(0, $num_args, 0);

			$iter = 1;
			while ($iter <= $iterations)
			{
				$vlist = array();
				foreach ($args_cur as $i => $v)
					array_push($vlist, $host_pattern_args[$i][$v]);

				array_push($result, vsprintf($host_format, $vlist));

				for ($i = 0; $i < $num_args; $i++)
				{
					if ($args_cur[$i] < $args_max[$i])
					{
						$args_cur[$i]++;
						break;
					}
					else
						$args_cur[$i] = 0;
				}
				$iter++;
			}
		}
		return $result;
	}
?>
